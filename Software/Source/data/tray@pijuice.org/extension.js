import GObject from 'gi://GObject';
import St from 'gi://St';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Clutter from 'gi://Clutter';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as PanelMenu from 'resource:///org/gnome/shell/ui/panelMenu.js';
import * as PopupMenu from 'resource:///org/gnome/shell/ui/popupMenu.js';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const PiJuiceTray = GObject.registerClass(
class PiJuiceTray extends PanelMenu.Button {
    _init() {
        super._init(0.0, 'PiJuice Tray Extension');

        this._process = null;
        this._refreshPprocess = null;
        this._dataInputStream = null;
        this._cancellable = null;

        // Create the status icon/label in the panel
        this._icon = new St.Icon({
            icon_name: 'battery-action-symbolic',
            style_class: 'system-status-icon',
        });
        this.add_child(this._icon);

        let settingsItem = new PopupMenu.PopupMenuItem('Settings');
        settingsItem.connect('activate', () => {
            this._settingsProcess();
        });
        this.menu.addMenuItem(settingsItem);

        let refreshItem = new PopupMenu.PopupMenuItem('Refresh');
        refreshItem.connect('activate', () => {
            this._refreshProcess();
        });
        this.menu.addMenuItem(refreshItem);

        let aboutItem = new PopupMenu.PopupMenuItem('About');
        aboutItem.connect('activate', () => {
            this._aboutProcess();
        });
        this.menu.addMenuItem(aboutItem);

        // Add a separator
        this.menu.addMenuItem(new PopupMenu.PopupSeparatorMenuItem());

        // Add status label
        this._statusItem = new PopupMenu.PopupMenuItem('Battery: <refresh required>', {
            reactive: false,
            can_focus: false
        });
        this.menu.addMenuItem(this._statusItem);

        this.refreshIntervalId = setInterval(this._refreshProcess.bind(this), 5000);
    }

    _aboutProcess() {
        if (this._process) {
            Main.notify('PiJuice Tray About', 'Process already running');
            return;
        }

        try {
            this._cancellable = new Gio.Cancellable();

            // Start the Python process
            this._process = Gio.Subprocess.new(
                ['/usr/bin/pijuice_tray.py', '--about'],
                Gio.SubprocessFlags.STDOUT_PIPE
            );

            // Get the output stream
            let stdout = this._process.get_stdout_pipe();
            this._dataInputStream = new Gio.DataInputStream({
                base_stream: stdout,
                close_base_stream: true
            });

            this._readOutput();

        } catch (e) {
            Main.notify('Extension Error', `Failed to start process: ${e.message}`);
            this._cleanup();
        }
    }

    _readOutput() {
        if (!this._dataInputStream) return;

        this._dataInputStream.read_line_async(
            GLib.PRIORITY_DEFAULT,
            this._cancellable,
            (stream, result) => {
                try {
                    let [line] = stream.read_line_finish_utf8(result);

                    if (line !== null) {
                        // Continue reading the next line
                        this._readOutput();
                    } else {
                        // Stream ended
                        this._cleanup();
                    }
                } catch (e) {
                    if (!e.matches(Gio.IOErrorEnum, Gio.IOErrorEnum.CANCELLED)) {
                        log(`Error reading output: ${e.message}`);
                    }
                    this._cleanup();
                }
            }
        );
    }

    _refreshProcess() {
        if (this._refreshPprocess) {
            return;
        }

        let outputData = '';
        let dataInputStream = null;
        let cancellable = null;

        const readStatus = () => {
            dataInputStream.read_line_async(
                GLib.PRIORITY_DEFAULT,
                cancellable,
                (stream, result) => {
                    try {
                        let [line] = stream.read_line_finish_utf8(result);

                        if (line !== null) {
                            outputData += line.trim() + '|';

                            // Continue reading the next line
                            readStatus();
                        } else {
                            // Stream ended
                            cleanup();
                            this._refreshStatusDisplay(outputData);
                        }
                    } catch (e) {
                        if (!e.matches(Gio.IOErrorEnum, Gio.IOErrorEnum.CANCELLED)) {
                            log(`Error reading output: ${e.message}`);
                        }
                        cleanup();
                    }
                }
            );
        }

        const cleanup = () => {
            if (cancellable) {
                cancellable.cancel();
                cancellable = null;
            }

            if (dataInputStream) {
                try {
                    dataInputStream.close(null);
                } catch (e) {
                    // Ignore close errors
                }
                dataInputStream = null;
            }

            if (this._refreshProcess) {
                try {
                    this._refreshProcess.force_exit();
                } catch (e) {
                    // Ignore force_exit errors
                }
                this._refreshProcess = null;
            }
        }

        try {
            cancellable = new Gio.Cancellable();

            // Start the Python process
            this._refreshProcess = Gio.Subprocess.new(
                ['/usr/bin/pijuice_tray.py', '--battery_status'],
                Gio.SubprocessFlags.STDOUT_PIPE
            );

            // Get the output stream
            let stdout = this._refreshProcess.get_stdout_pipe();
            dataInputStream = new Gio.DataInputStream({
                base_stream: stdout,
                close_base_stream: true
            });

            let outputData = '';

            readStatus();

        } catch (e) {
            Main.notify('Extension Error', `Failed to start process: ${e.message}`);
            cleanup();
        }
    }

    _refreshStatusDisplay(output) {
        let pijuice_status = output.match(/\|Status:([^|]*)/)[1];
        let battery = output.match(/\|Battery:([^|]*)/)[1];

        this._statusItem.label.text = pijuice_status + ', ' + battery + '%';

        if (pijuice_status == "No battery detected") {
            this._icon.icon_name = 'battery-action-symbolic';
        } else if (pijuice_status == "Running on battery") {
            this._icon.icon_name = 'battery-level-'+Math.round(battery)+'-symbolic';
        } else {
            this._icon.icon_name = 'battery-level-'+Math.round(battery)+'-plugged-in-symbolic';
        }

    }

    _settingsProcess() {
        if (this._process) {
            Main.notify('PiJuice Settings', 'Process already running');
            return;
        }

        try {
            this._cancellable = new Gio.Cancellable();

            // Start the Python process
            this._process = Gio.Subprocess.new(
                ['/usr/bin/pijuice_gui'],
                Gio.SubprocessFlags.STDOUT_PIPE
            );

            // Get the output stream
            let stdout = this._process.get_stdout_pipe();
            this._dataInputStream = new Gio.DataInputStream({
                base_stream: stdout,
                close_base_stream: true
            });

            this._readOutput();

        } catch (e) {
            Main.notify('Extension Error', `Failed to start process: ${e.message}`);
            this._cleanup();
        }
    }

    _cleanup() {
        if (this._cancellable) {
            this._cancellable.cancel();
            this._cancellable = null;
        }

        if (this._dataInputStream) {
            try {
                this._dataInputStream.close(null);
            } catch (e) {
                // Ignore close errors
            }
            this._dataInputStream = null;
        }

        if (this._process) {
            try {
                this._process.force_exit();
            } catch (e) {
                // Ignore force_exit errors
            }
            this._process = null;
        }
    }

    destroy() {
        this._cleanup();
        clearInterval(this.refreshIntervalId);
        super.destroy();
    }
});

export default class SimpleExtension extends Extension {
    enable() {
        this._indicator = new PiJuiceTray();
        Main.panel.addToStatusArea(this.uuid, this._indicator);
    }

    disable() {
        if (this._indicator) {
            this._indicator.destroy();
            this._indicator = null;
        }
    }
}
