#!/usr/bin/env python3
"""
GUI Serial Monitor with Build Trigger
Tkinter-based interface for serial monitoring and PlatformIO builds
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import subprocess
import os
from datetime import datetime
import queue

class SerialMonitorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Serial Monitor with Build Trigger")
        self.root.geometry("800x600")
        
        # Serial connection variables
        self.serial_connection = None
        self.is_monitoring = False
        self.monitor_thread = None
        self.build_in_progress = False
        
        # Queue for thread-safe GUI updates
        self.message_queue = queue.Queue()
        
        self.setup_gui()
        self.refresh_ports()
        self.process_queue()
        
    def setup_gui(self):
        """Setup the GUI layout"""
        # Main frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)
        main_frame.columnconfigure(1, weight=1)
        main_frame.rowconfigure(2, weight=1)
        
        # Connection frame
        conn_frame = ttk.LabelFrame(main_frame, text="Connection", padding="5")
        conn_frame.grid(row=0, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=(0, 10))
        conn_frame.columnconfigure(1, weight=1)
        
        # Port selection
        ttk.Label(conn_frame, text="Port:").grid(row=0, column=0, sticky=tk.W, padx=(0, 5))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(0, 5))
        
        ttk.Button(conn_frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=(0, 5))
        
        # Baud rate
        ttk.Label(conn_frame, text="Baud:").grid(row=0, column=3, sticky=tk.W, padx=(5, 5))
        self.baud_var = tk.StringVar(value="115200")
        baud_combo = ttk.Combobox(conn_frame, textvariable=self.baud_var, width=10)
        baud_combo['values'] = ('9600', '19200', '38400', '57600', '115200', '230400', '460800', '921600')
        baud_combo.grid(row=0, column=4, padx=(0, 5))
        
        # Control buttons frame
        control_frame = ttk.Frame(main_frame)
        control_frame.grid(row=1, column=0, columnspan=3, sticky=(tk.W, tk.E), pady=(0, 10))
        
        self.connect_btn = ttk.Button(control_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=0, padx=(0, 5))
        
        self.build_btn = ttk.Button(control_frame, text="Build & Upload", command=self.trigger_build, state="disabled")
        self.build_btn.grid(row=0, column=1, padx=(0, 5))
        
        self.reset_btn = ttk.Button(control_frame, text="Reset Device", command=self.reset_device, state="disabled")
        self.reset_btn.grid(row=0, column=2, padx=(0, 5))
        
        self.clear_btn = ttk.Button(control_frame, text="Clear Log", command=self.clear_log)
        self.clear_btn.grid(row=0, column=3, padx=(0, 5))
        
        # Status label
        self.status_var = tk.StringVar(value="Disconnected")
        self.status_label = ttk.Label(control_frame, textvariable=self.status_var, foreground="red")
        self.status_label.grid(row=0, column=4, padx=(20, 0))
        
        # Log area
        log_frame = ttk.LabelFrame(main_frame, text="Serial Output", padding="5")
        log_frame.grid(row=2, column=0, columnspan=3, sticky=(tk.W, tk.E, tk.N, tk.S))
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, state="disabled")
        self.log_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure text tags for different message types
        self.log_text.tag_configure("timestamp", foreground="gray")
        self.log_text.tag_configure("build", foreground="blue", font=("TkDefaultFont", 9, "bold"))
        self.log_text.tag_configure("error", foreground="red")
        self.log_text.tag_configure("success", foreground="green")
        
    def refresh_ports(self):
        """Refresh the list of available serial ports"""
        ports = serial.tools.list_ports.comports()
        port_list = []
        
        for port in ports:
            # Prioritize ESP32/ESP8266 devices
            if any(keyword in port.description.lower() for keyword in ['esp', 'cp210', 'ch340', 'ftdi']):
                port_list.insert(0, f"{port.device} - {port.description}")
            else:
                port_list.append(f"{port.device} - {port.description}")
        
        self.port_combo['values'] = port_list
        if port_list and not self.port_var.get():
            self.port_combo.current(0)
    
    def get_selected_port(self):
        """Extract port name from the selected combo box value"""
        selection = self.port_var.get()
        if selection:
            return selection.split(' - ')[0]
        return None
    
    def log_message(self, message, tag=None):
        """Add a message to the log (thread-safe)"""
        self.message_queue.put(("append", message, tag))
    
    def update_current_line(self, message, tag=None):
        """Update the current line in the log (for progress bars)"""
        self.message_queue.put(("update", message, tag))
    
    def process_queue(self):
        """Process messages from the queue and update GUI"""
        try:
            while True:
                queue_item = self.message_queue.get_nowait()
                action = queue_item[0]
                message = queue_item[1]
                tag = queue_item[2] if len(queue_item) > 2 else None
                
                self.log_text.config(state="normal")
                
                if action == "append":
                    # Normal append operation
                    if tag:
                        self.log_text.insert(tk.END, message + "\n", tag)
                    else:
                        self.log_text.insert(tk.END, message + "\n")
                elif action == "update":
                    # Update current line (for progress bars)
                    try:
                        # Get the last line and replace it
                        last_line_start = self.log_text.index("end-2l linestart")
                        last_line_end = self.log_text.index("end-2l lineend")
                        
                        # Always replace the last line content
                        self.log_text.delete(last_line_start, last_line_end)
                        if tag:
                            self.log_text.insert(last_line_start, message, tag)
                        else:
                            self.log_text.insert(last_line_start, message)
                    except tk.TclError:
                        # If there's no content yet, just append
                        if tag:
                            self.log_text.insert(tk.END, message, tag)
                        else:
                            self.log_text.insert(tk.END, message)
                
                self.log_text.config(state="disabled")
                self.log_text.see(tk.END)
                
        except queue.Empty:
            pass
        except tk.TclError:
            # Handle text widget errors gracefully
            pass
        
        # Schedule next check
        self.root.after(10, self.process_queue)  # Very fast updates for real-time progress
    
    def toggle_connection(self):
        """Toggle serial connection"""
        if self.is_monitoring:
            self.disconnect_serial()
        else:
            self.connect_serial()
    
    def connect_serial(self):
        """Connect to the selected serial port"""
        port = self.get_selected_port()
        if not port:
            messagebox.showerror("Error", "Please select a serial port")
            return
        
        try:
            baudrate = int(self.baud_var.get())
            self.serial_connection = serial.Serial(
                port=port,
                baudrate=baudrate,
                timeout=1
            )
            
            self.is_monitoring = True
            self.monitor_thread = threading.Thread(target=self.monitor_serial, daemon=True)
            self.monitor_thread.start()
            
            self.connect_btn.config(text="Disconnect")
            self.build_btn.config(state="normal")
            self.reset_btn.config(state="normal")
            self.status_var.set("Connected")
            self.status_label.config(foreground="green")
            
            self.log_message(f"Connected to {port} at {baudrate} baud", "success")
            
        except serial.SerialException as e:
            messagebox.showerror("Connection Error", f"Failed to connect to {port}: {e}")
        except ValueError:
            messagebox.showerror("Error", "Invalid baud rate")
    
    def disconnect_serial(self):
        """Disconnect from serial port"""
        self.is_monitoring = False
        
        if self.monitor_thread:
            self.monitor_thread.join(timeout=2)
        
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
        
        self.connect_btn.config(text="Connect")
        self.build_btn.config(state="disabled")
        self.reset_btn.config(state="disabled")
        self.status_var.set("Disconnected")
        self.status_label.config(foreground="red")
        
        self.log_message("Serial connection closed", "build")
    
    def monitor_serial(self):
        """Monitor serial port for incoming data"""
        while self.is_monitoring:
            try:
                if self.serial_connection and self.serial_connection.is_open:
                    if self.serial_connection.in_waiting > 0:
                        data = self.serial_connection.readline().decode('utf-8', errors='ignore').strip()
                        if data:
                            timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                            self.log_message(f"[{timestamp}] {data}")
                else:
                    time.sleep(0.1)
            except serial.SerialException as e:
                self.log_message(f"Serial error: {e}", "error")
                break
            except Exception as e:
                self.log_message(f"Unexpected error: {e}", "error")
                break
    
    def reset_device(self):
        """Reset the device by toggling CTS pin"""
        if not self.serial_connection or not self.serial_connection.is_open:
            messagebox.showerror("Error", "No serial connection available")
            return
        
        try:
            self.log_message("Resetting device via CTS pin...", "build")
            
            # Toggle CTS pin to reset the device
            # CTS low = reset active, CTS high = reset inactive
            self.serial_connection.rts = False  # Set RTS low (reset active)
            time.sleep(0.1)  # Hold reset for 100ms
            self.serial_connection.rts = True   # Set RTS high (reset inactive)
            
            self.log_message("Device reset complete", "success")
            
        except Exception as e:
            self.log_message(f"Reset failed: {e}", "error")
            messagebox.showerror("Reset Error", f"Failed to reset device: {e}")
    
    def trigger_build(self):
        """Trigger PlatformIO build and upload in a separate thread"""
        if self.build_in_progress:
            messagebox.showwarning("Build in Progress", "A build is already in progress...")
            return
        
        # Clear log before starting build
        self.clear_log()
        
        threading.Thread(target=self._build_worker, daemon=True).start()
    
    def _build_worker(self):
        """Worker thread for build process"""
        self.build_in_progress = True
        
        # Update UI
        self.root.after(0, lambda: self.build_btn.config(state="disabled", text="Building..."))
        
        self.log_message("=" * 50, "build")
        self.log_message("TRIGGERING BUILD AND UPLOAD", "build")
        self.log_message("=" * 50, "build")
        
        # Disconnect serial
        was_connected = self.is_monitoring
        if was_connected:
            self.log_message("Disconnecting serial...", "build")
            self.root.after(0, self.disconnect_serial)
            time.sleep(1)
        
        try:
            # Change to project root directory
            project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            original_dir = os.getcwd()
            os.chdir(project_root)
            
            # Run PlatformIO upload with real-time output
            self.log_message("Running PlatformIO upload...", "build")
            
            process = subprocess.Popen(
                ["platformio", "run", "-e", "esp12f", "--target", "upload"],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=0,  # Unbuffered for character-by-character
                universal_newlines=True
            )
            
            # Read output character by character to handle progress bars
            current_line = ""
            while True:
                char = process.stdout.read(1)
                if char == '' and process.poll() is not None:
                    break
                if char:
                    if char == '\r':
                        # Carriage return - update current line in place
                        if current_line:
                            self.update_current_line(current_line)
                        current_line = ""
                    elif char == '\n':
                        # New line - add to log and start new line
                        if current_line:
                            self.log_message(current_line)
                        current_line = ""
                    else:
                        current_line += char
                        # For connecting progress, update immediately as characters come in
                        if char in '._ ' and ('Connecting' in current_line or 'Writing' in current_line):
                            self.update_current_line(current_line)
            
            # Handle any remaining content
            if current_line.strip():
                self.log_message(current_line.strip())
            
            return_code = process.poll()
            os.chdir(original_dir)
            
            if return_code == 0:
                self.log_message("[OK] Upload successful!", "success")
            else:
                self.log_message("[ERROR] Upload failed!", "error")
                        
        except FileNotFoundError:
            self.log_message("[ERROR] PlatformIO not found! Make sure it's installed and in PATH", "error")
        except Exception as e:
            self.log_message(f"[ERROR] Build error: {e}", "error")
        
        # Wait for device to reset
        self.log_message("Waiting for device to reset...", "build")
        time.sleep(3)
        
        # Reconnect serial if it was connected before
        if was_connected:
            self.log_message("Reconnecting serial...", "build")
            self.root.after(0, self.connect_serial)
        
        self.log_message("=" * 50, "build")
        self.log_message("BUILD COMPLETE", "build")
        self.log_message("=" * 50, "build")
        
        # Update UI
        self.build_in_progress = False
        self.root.after(0, lambda: (
            self.build_btn.config(state="normal" if self.is_monitoring else "disabled", text="Build & Upload"),
            self.reset_btn.config(state="normal" if self.is_monitoring else "disabled")
        ))
    
    def clear_log(self):
        """Clear the log text area"""
        self.log_text.config(state="normal")
        self.log_text.delete(1.0, tk.END)
        self.log_text.config(state="disabled")

def main():
    root = tk.Tk()
    app = SerialMonitorGUI(root)
    
    try:
        root.mainloop()
    except KeyboardInterrupt:
        pass

if __name__ == "__main__":
    main()