import os
import subprocess
Import("env")

def kill_serial_process(source, target, env):
    print("\n--- [Custom Script] Hunting for locked serial ports... ---")
    
    # Search for anything matching "usbserial-0001" to catch both cu and tty
    port_keyword = "usbserial-0001"
    
    try:
        # Run lsof to find any process using a file containing our keyword
        # 'pgrep' or 'lsof -c python' could work, but checking the file path is safest
        cmd = f"lsof | grep {port_keyword} | awk '{{print $2}}'"
        pid_output = subprocess.check_output(cmd, shell=True).decode("utf-8").strip()
        
        if pid_output:
            # Split by newlines and filter out duplicates
            pids = set(pid_output.split("\n"))
            for pid in pids:
                if pid.isdigit():
                    print(f"Found process {pid} locking the port. Terminating...")
                    # Send a harsh kill signal (-9) to force it to close
                    subprocess.call(["kill", "-9", pid])
            print("Port cleared successfully!\n")
        else:
            print("No active processes locking the port.\n")
            
    except Exception as e:
        print(f"Script encountered a hiccup checking ports: {e}\n")

# Hook the function to run BEFORE the upload action
env.AddPreAction("upload", kill_serial_process)