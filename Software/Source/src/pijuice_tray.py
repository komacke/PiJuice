#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import print_function, division

import os
import os.path
import sys
import argparse
from signal import signal, SIGUSR1, SIGUSR2
import json
import gi
gi.require_version('Gtk', '4.0')
gi.require_version('Adw', '1')
from gi.repository import Gtk as gtk
from gi.repository import GLib as glib
from gi.repository import Gio as gio
from gi.repository import Adw as adw

from pijuice import PiJuice, get_versions

I2C_ADDRESS_DEFAULT = 0x14
I2C_BUS_DEFAULT = 1
REFRESH_INTERVAL = 5000
CHECK_SIGNAL_INTERVAL = 200
ICON_DIR = '/usr/share/pijuice/data/images'
TRAY_PID_FILE = '/run/pijuice/pijuice_tray.pid'
configPath = '/var/lib/pijuice/pijuice_config.JSON'

class PiJuiceStatusTray(adw.Application):

    def __init__(self):
        super().__init__(application_id='org.pijuice.tray', 
                         flags=gio.ApplicationFlags.FLAGS_NONE)
        
        self.check_args()

        self.window = None
        self.status_window = None
        self.refresh_err = 0
        self.current_battery_level = 0
        self.current_icon_file = None
        self.status_text = ''
        
        # Connect application signals
        self.connect('activate', self.on_activate)
        self.connect('startup', self.on_startup)

    def check_args(self):
        parser = argparse.ArgumentParser()
        parser.add_argument('--about', action='store_true', help="Show 'About' popup")
        parser.add_argument('--battery_status', action='store_true', help='Get battery status to stdout')
        self.args = parser.parse_args()

    def on_startup(self, app):
        """Called when the application starts up"""
        self.init_pijuice_interface()
        
        # Set up signal handlers
        signal(SIGUSR1, self.receive_signal)
        signal(SIGUSR2, self.receive_signal)
        
        # Create actions
        self.create_actions()
        
        # Current arguments are just one and done so don't start timers
        if not any(vars(self.args).values()):
            glib.timeout_add(REFRESH_INTERVAL, self.refresh_status)
            glib.timeout_add(CHECK_SIGNAL_INTERVAL, self.check_signum)

    def on_activate(self, app):
        """Called when the application is activated"""
        if not self.window:
            self.create_status_window()

        if self.args.about:
            self.activate_action("about", None)
        else:
            self.window.present()

    def create_actions(self):
        """Create application actions"""
        # Settings action
        settings_action = gio.SimpleAction.new('settings', None)
        settings_action.connect('activate', self.on_settings_activate)
        self.add_action(settings_action)
        self.settings_action = settings_action
        
        # About action
        about_action = gio.SimpleAction.new('about', None)
        about_action.connect('activate', self.on_about_activate)
        self.add_action(about_action)
        
        # Refresh action
        refresh_action = gio.SimpleAction.new('refresh', None)
        refresh_action.connect('activate', self.on_refresh_activate)
        self.add_action(refresh_action)
        
        # Quit action
        quit_action = gio.SimpleAction.new('quit', None)
        quit_action.connect('activate', self.on_quit_activate)
        self.add_action(quit_action)

    def create_status_window(self):
        """Create the main status window"""
        self.window = gtk.ApplicationWindow(application=self)
        self.window.set_title("PiJuice Status")
        self.window.set_default_size(300, 200)

        # Create header bar
        header = gtk.HeaderBar()
        self.window.set_titlebar(header)
        
        # Add menu button
        menu_button = gtk.MenuButton()
        menu_button.set_icon_name("open-menu-symbolic")
        header.pack_end(menu_button)
        
        # Create menu
        menu = gio.Menu()
        menu.append("Settings", "app.settings")
        menu.append("About", "app.about")
        menu.append("Refresh", "app.refresh")
        menu.append("Quit", "app.quit")
        
        menu_button.set_menu_model(menu)
        
        # Create main content
        self.create_main_content()
        
        # Handle window close
        self.window.connect('close-request', self.on_window_close)

    def create_main_content(self):
        """Create the main window content"""
        box = gtk.Box(orientation=gtk.Orientation.VERTICAL, spacing=10)
        box.set_margin_top(20)
        box.set_margin_bottom(20)
        box.set_margin_start(20)
        box.set_margin_end(20)
        
        # Battery icon
        self.battery_image = gtk.Image()
        self.battery_image.set_pixel_size(64)
        box.append(self.battery_image)
        
        # Battery level label
        self.battery_label = gtk.Label()
        self.battery_label.set_markup("<span size='large' weight='bold'>---%</span>")
        box.append(self.battery_label)
        
        # Status label
        self.status_label = gtk.Label()
        self.status_label.set_text("Checking status...")
        box.append(self.status_label)
        
        self.window.set_child(box)
        
        # Initial refresh
        if not any(vars(self.args).values()):
            self.refresh_status()

    def init_pijuice_interface(self):
        """Initialize PiJuice interface"""
        try:
            addr = I2C_ADDRESS_DEFAULT
            bus = I2C_BUS_DEFAULT

            configData = {}
            if os.path.exists(configPath):
                with open(configPath, 'r') as outputConfig:
                    config_dict = json.load(outputConfig)
                    configData.update(config_dict)
                    
                if 'board' in configData and 'general' in configData['board']:
                    if 'i2c_addr' in configData['board']['general']:
                        addr = int(configData['board']['general']['i2c_addr'], 16)
                    if 'i2c_bus' in configData['board']['general']:
                        bus = configData['board']['general']['i2c_bus']

            self.pijuice = PiJuice(bus, addr)
        except Exception as e:
            print(f"Failed to initialize PiJuice interface: {e}")
            # Don't exit immediately, allow GUI to show error

    def on_settings_activate(self, action, param):
        """Handle settings menu activation"""
        os.system("/usr/bin/pijuice_gui &")

    def about_message(self):
        sw_version, fw_version, os_version = get_versions()
        if fw_version is None:
            fw_version = "No connection to PiJuice"
            
        return "\n".join([
            f"Software version: {sw_version}",
            f"Firmware version: {fw_version}",
            f"OS version: {os_version}",
        ])

    def get_battery_status(self):
        self.init_pijuice_interface()
        self.refresh_status()
        print(f"Battery:{self.current_battery_level}")
        try:
            if self.status_text == '':
                self.status_text = self.get_status_text(self.status)
            print(f"Status:{self.status_text}")
        except:
            print(f"Status:Error reading battery status")

    def on_about_activate(self, action, param):
        """Handle about menu activation"""
        action.set_enabled(False)

        message = self.about_message()
        
        # Create about dialog
        dialog = gtk.AlertDialog(
            modal=True,
            message="About PiJuice",
            detail=message
        )
        if self.args.about:
            callback=self.on_quit_activate
        else:
            callback=lambda source, result: action.set_enabled(True)
        dialog.choose(
            parent=self.window,
            cancellable=None,
            callback=callback
        )

    def on_refresh_activate(self, action, param):
        """Handle refresh menu activation"""
        self.refresh_status()

    def on_quit_activate(self, action, param):
        """Handle quit menu activation"""
        self.quit()

    def on_window_close(self, window):
        """Handle window close request"""
        # Hide window instead of closing to keep tray functionality
        window.set_visible(False)
        return True  # Prevent default close behavior

    def check_signum(self):
        """Check for signals - GTK4 compatible version"""
        global sig
        if 'sig' in globals() and sig > 0:
            if sig == SIGUSR1:
                self.settings_action.set_enabled(False)
                sig = -1
            elif sig == SIGUSR2:
                self.settings_action.set_enabled(True)
                sig = -1
            else:
                sig = -1
        return True

    def refresh_status(self):
        """Refresh battery status and update display"""
        try:
            if not hasattr(self, 'pijuice'):
                self.init_pijuice_interface()
                return True

            self.charge = self.pijuice.status.GetChargeLevel()
            
            if self.charge['error'] == 'NO_ERROR':
                b_level = self.charge['data']
                self.current_battery_level = b_level
                print(f'{b_level}%')
            else:
                print(f"Charge level error: {self.charge['error']}")
                self.init_pijuice_interface()
                return True

            # Determine icon file
            b_file = ICON_DIR + '/battery_near_full.png'

            status = self.pijuice.status.GetStatus()
            if status['error'] == 'NO_ERROR':
                self.status = status['data']
                self.status_text = self.get_status_text(self.status)

                if self.status['battery'] == 'NOT_PRESENT':
                    if self.status['powerInput'] != 'NOT_PRESENT':
                        b_file = ICON_DIR + '/no-bat-in-0.png'
                    else:
                        b_file = ICON_DIR + '/no-bat-rpi-0.png'
                elif self.status['battery'] == 'CHARGING_FROM_IN' or self.status['powerInput'] != 'NOT_PRESENT':
                    b_file = ICON_DIR + '/bat-in-' + str((b_level//10)*10) + '.png'
                elif self.status['battery'] == 'CHARGING_FROM_5V_IO' or self.status['powerInput5vIo'] != 'NOT_PRESENT':
                    b_file = ICON_DIR + '/bat-rpi-' + str((b_level//10)*10) + '.png'
                else:
                    b_file = ICON_DIR + '/bat-' + str((b_level//10)*10) + '.png'
            else:
                b_file = ICON_DIR + '/connection-error.png'
                self.status_text = f"Connection error: {status['error']}"

            # Update GUI if window exists
            if self.window:
                self.update_display(b_level, b_file, self.status_text)
            
            self.current_icon_file = b_file
            self.refresh_err = 0

        except Exception as e:
            print(f'Refresh error: {e}')
            self.refresh_err += 1
            if self.refresh_err > 4:
                print("Too many refresh errors, exiting")
                self.quit()
                return False
            
            if self.window:
                self.status_label.set_text("Connection error")
        
        return True

    def get_status_text(self, status):
        """Generate human-readable status text"""
        battery_status = status.get('battery', 'UNKNOWN')
        power_input = status.get('powerInput', 'NOT_PRESENT')
        power_input_5v = status.get('powerInput5vIo', 'NOT_PRESENT')
        
        if battery_status == 'NOT_PRESENT':
            return "No battery detected"
        elif battery_status == 'CHARGING_FROM_IN':
            return "Charging from input"
        elif battery_status == 'CHARGING_FROM_5V_IO':
            return "Charging from 5V IO"
        elif power_input != 'NOT_PRESENT':
            return "External power connected"
        else:
            return "Running on battery"

    def update_display(self, battery_level, icon_file, status_text):
        """Update the GUI display"""
        # Update battery level
        self.battery_label.set_markup(f"<span size='large' weight='bold'>{battery_level}%</span>")
        
        # Update status
        self.status_label.set_text(status_text)
        
        # Update icon
        if os.path.exists(icon_file):
            self.battery_image.set_from_file(icon_file)
        else:
            print(f"Icon file not found: {icon_file}")
            # Set a fallback icon
            self.battery_image.set_from_icon_name("battery-symbolic")

    def receive_signal(self, signum, stack):
        """Signal handler"""
        global sig
        if signum == SIGUSR1 or signum == SIGUSR2:
            sig = signum
        else:
            sig = -1


def main():
    """Main entry point"""
    global sig
    sig = -1

    # Make our pid available for pijuice_gui
    pid = os.getpid()
    
    # Create directory if it doesn't exist
    pid_dir = os.path.dirname(TRAY_PID_FILE)
    if not os.path.exists(pid_dir):
        os.makedirs(pid_dir, exist_ok=True)
    
    try:
        with open(TRAY_PID_FILE, 'w') as f:
            f.write(str(pid))
        # Allow other user to overwrite this file
        if oct(os.stat(TRAY_PID_FILE).st_mode & 0o777) != '0o666':
            os.chmod(TRAY_PID_FILE, 0o666)
    except Exception as e:
        print(f"Warning: Could not create PID file: {e}")

    # Initialize Adwaita (modern GNOME styling)
    adw.init()
    
    # Create and run application
    app = PiJuiceStatusTray()

    # if getting battery status, just return - don't run
    if app.args.battery_status:
        app.get_battery_status()
        return

    return app.run()


if __name__ == '__main__':
    sys.exit(main())
