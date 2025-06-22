// Global application state
let currentState = null;

// Initialize app when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    initializeApp();
});

function initializeApp() {
    console.log('Initializing WiFi Device Manager app');
    
    // Listen for WebSocket state updates
    if (window.wsManager) {
        window.wsManager.addListener(handleStateUpdate);
    }
    
    // Add connection status indicator to header if not present
    addConnectionStatusIndicator();
    
    // Initialize Material Design components
    if (window.componentHandler) {
        window.componentHandler.upgradeDom();
    }
}

function handleStateUpdate(type, data) {
    if (type === 'state-update') {
        currentState = data;
        updateUI(data);
    }
}

function updateUI(state) {
    console.log('Updating UI with new state', state);
    
    // Update device cards
    updateDeviceCards(state.devices);
    
    // Update system stats
    updateSystemStats(state.systemStats);
    
    // Update any other UI elements
    updateLastUpdateTime();
}

function updateDeviceCards(devices) {
    // Convert Map to Array if needed
    const deviceArray = devices instanceof Map ? Array.from(devices.values()) : 
                       Array.isArray(devices) ? devices : Object.values(devices);
    
    deviceArray.forEach(device => {
        updateDeviceCard(device);
    });
}

function updateDeviceCard(device) {
    // Update device alias
    const aliasElement = document.querySelector(`[data-device-id="${device.id}"] .device-alias`);
    if (aliasElement) {
        aliasElement.textContent = device.alias;
    }
    
    // Update online status
    const statusElements = document.querySelectorAll(`[data-device-id="${device.id}"] .device-status`);
    statusElements.forEach(element => {
        const statusIcon = element.querySelector('.material-icons');
        const statusText = statusIcon ? statusIcon.nextSibling : null;
        
        if (statusIcon) {
            statusIcon.textContent = device.isOnline ? 'wifi' : 'wifi_off';
            statusIcon.parentElement.className = device.isOnline ? 'device-online' : 'device-offline';
        }
        
        if (statusText) {
            statusText.textContent = device.isOnline ? ' Online' : ' Offline';
        }
    });
    
    // Update output state
    const outputElements = document.querySelectorAll(`[data-device-id="${device.id}"] .output-state`);
    outputElements.forEach(element => {
        element.textContent = device.currentOutput ? 'ON' : 'OFF';
        element.className = device.currentOutput ? 'device-online' : 'device-offline';
    });
    
    // Update pending commands count
    const pendingElements = document.querySelectorAll(`[data-device-id="${device.id}"] .pending-commands`);
    pendingElements.forEach(element => {
        const count = device.pendingCommandCount || 0;
        if (count > 0) {
            element.style.display = 'block';
            element.querySelector('.command-count').textContent = count;
        } else {
            element.style.display = 'none';
        }
    });
}

function updateSystemStats(stats) {
    // Update total devices
    const totalElement = document.querySelector('.total-devices');
    if (totalElement) {
        totalElement.textContent = stats.totalDevices;
    }
    
    // Update online devices
    const onlineElement = document.querySelector('.online-devices');
    if (onlineElement) {
        onlineElement.textContent = stats.onlineDevices;
    }
    
    // Update offline devices
    const offlineElement = document.querySelector('.offline-devices');
    if (offlineElement) {
        offlineElement.textContent = stats.totalDevices - stats.onlineDevices;
    }
    
    // Update pending commands
    const pendingElement = document.querySelector('.pending-commands');
    if (pendingElement) {
        pendingElement.textContent = stats.pendingCommands;
    }
}

function updateLastUpdateTime() {
    const timeElement = document.querySelector('.last-update-time');
    if (timeElement) {
        timeElement.textContent = new Date().toLocaleTimeString();
    }
}

// Device control functions
function controlDevice(deviceId, action, scheduleDelay = null) {
    const payload = { action };
    if (scheduleDelay) {
        payload.scheduleDelay = scheduleDelay;
    }
    
    fetch(`/api/devices/${deviceId}/control`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(payload)
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            let actionText = action;
            // Provide user-friendly text for valve commands
            if (action === 'valve-open') actionText = 'open valve';
            if (action === 'valve-close') actionText = 'close valve';
            showNotification(`Command ${actionText} sent to device`);
        } else {
            showNotification(`Failed to send command: ${data.error}`, 'error');
        }
    })
    .catch(error => {
        console.error('Error controlling device:', error);
        showNotification('Failed to send command', 'error');
    });
}

function editDeviceName(deviceId) {
    const aliasElement = document.querySelector(`[data-device-id="${deviceId}"]`);
    if (!aliasElement) return;
    
    const currentAlias = aliasElement.textContent.trim();
    const newAlias = prompt('Enter new device name:', currentAlias);
    
    if (newAlias && newAlias.trim() !== currentAlias) {
        renameDevice(deviceId, newAlias.trim());
    }
}

function renameDevice(deviceId, newAlias) {
    fetch(`/api/devices/${deviceId}/rename`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ alias: newAlias })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification(`Device renamed to "${newAlias}"`);
        } else {
            showNotification(`Failed to rename device: ${data.error}`, 'error');
        }
    })
    .catch(error => {
        console.error('Error renaming device:', error);
        showNotification('Failed to rename device', 'error');
    });
}

// Utility functions
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    
    // Style the notification
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${type === 'error' ? '#f44336' : '#4caf50'};
        color: white;
        padding: 12px 24px;
        border-radius: 4px;
        box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        z-index: 10000;
        animation: slideIn 0.3s ease-out;
    `;
    
    // Add animation styles if not already present
    if (!document.querySelector('#notification-styles')) {
        const styles = document.createElement('style');
        styles.id = 'notification-styles';
        styles.textContent = `
            @keyframes slideIn {
                from { transform: translateX(100%); opacity: 0; }
                to { transform: translateX(0); opacity: 1; }
            }
            @keyframes slideOut {
                from { transform: translateX(0); opacity: 1; }
                to { transform: translateX(100%); opacity: 0; }
            }
        `;
        document.head.appendChild(styles);
    }
    
    document.body.appendChild(notification);
    
    // Remove notification after 3 seconds
    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease-in';
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 300);
    }, 3000);
}

function addConnectionStatusIndicator() {
    const header = document.querySelector('.mdl-layout__header-row');
    if (header && !document.getElementById('connection-status')) {
        const statusContainer = document.createElement('div');
        statusContainer.innerHTML = `
            <div id="connection-status" class="device-online" style="margin-left: 16px; font-size: 12px;">
                Connected
            </div>
        `;
        header.appendChild(statusContainer);
    }
}

function formatTimeAgo(timestamp) {
    const now = new Date();
    const time = new Date(timestamp);
    const diffMs = now - time;
    const diffMins = Math.floor(diffMs / 60000);
    const diffHours = Math.floor(diffMins / 60);
    const diffDays = Math.floor(diffHours / 24);
    
    if (diffMins < 1) return 'Just now';
    if (diffMins < 60) return `${diffMins}m ago`;
    if (diffHours < 24) return `${diffHours}h ago`;
    return `${diffDays}d ago`;
}

// Export functions for global use
window.controlDevice = controlDevice;
window.editDeviceName = editDeviceName;
window.renameDevice = renameDevice;
window.showNotification = showNotification;