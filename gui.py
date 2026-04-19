import tkinter as tk
import serial
import threading
import queue
import sys

# --- CONFIGURATION ---
SERIAL_PORT = 'COM3'  # Update this to your Arduino's port
BAUD_RATE = 250000

class MaimaiVisualizer(tk.Tk):
    def __init__(self, data_queue, cmd_queue):
        super().__init__()
        self.title("maimai Sensor Grid Visualizer")
        self.configure(padx=20, pady=20, bg="#2c3e50")
        self.data_queue = data_queue
        self.cmd_queue = cmd_queue
        
        # UI Configuration
        self.active_color = "#27ae60"  # Green when touched
        self.inactive_color = "#34495e" # Dark blue/gray when released
        self.disabled_color = "#1a252f" # Darker gray for inactive cells
        self.text_color = "#ecf0f1"
        
        self.cells = {}
        self.states = {}
        
        self.create_grid()
        
        # Bind the 'R' key to trigger a hardware reset
        self.bind('<r>', self.send_reset)
        self.bind('<R>', self.send_reset)
        
        # Add instruction label
        tk.Label(self, text="Press 'R' on your keyboard to soft reset and recalibrate the MPR121 baselines.", 
                 font=("Arial", 10, "italic"), bg="#2c3e50", fg="#bdc3c7").grid(row=6, column=0, columnspan=9, pady=(20, 0))

        self.poll_queue()

    def send_reset(self, event=None):
        self.cmd_queue.put(b'R')
        print("Sent reset command to Arduino.")

    def create_grid(self):
        rows = ['A', 'B', 'C', 'D', 'E']
        
        for col in range(1, 9):
            tk.Label(self, text=str(col), font=("Arial", 14, "bold"), bg="#2c3e50", fg=self.text_color).grid(row=0, column=col, padx=5, pady=5)
            
        for r_idx, row in enumerate(rows):
            tk.Label(self, text=row, font=("Arial", 14, "bold"), bg="#2c3e50", fg=self.text_color).grid(row=r_idx+1, column=0, padx=5, pady=5)
            
            for col in range(1, 9):
                cell_id = f"{row}{col}"
                
                # Increased cell height to fit the delta (D) row
                frame = tk.Frame(self, width=90, height=125, bg=self.inactive_color, highlightbackground="#7f8c8d", highlightthickness=1)
                frame.grid(row=r_idx+1, column=col, padx=5, pady=5)
                frame.grid_propagate(False) 
                
                if row == 'C' and col >= 3:
                    frame.configure(bg=self.disabled_color)
                    lbl = tk.Label(frame, text="N/A", font=("Arial", 9), bg=self.disabled_color, fg="#7f8c8d")
                    lbl.place(relx=0.5, rely=0.5, anchor="center")
                else:
                    lbl = tk.Label(frame, text=f"{cell_id}\nF: -\nB: -\nD: -\nTT: -\nRT: -", font=("Arial", 9), bg=self.inactive_color, fg=self.text_color)
                    lbl.place(relx=0.5, rely=0.5, anchor="center")
                    self.cells[cell_id] = {'frame': frame, 'label': lbl}
                    self.states[cell_id] = False 

    def poll_queue(self):
        while not self.data_queue.empty():
            try:
                line = self.data_queue.get_nowait()
                self.process_line(line)
            except queue.Empty:
                break
                
        self.after(20, self.poll_queue)

    def process_line(self, line):
        tokens = line.strip().split()
        
        for token in tokens:
            try:
                cell_id, data = token.split(':')
                fd_str, bv_str, tt_str, rt_str = data.split(',')
                
                fd = int(fd_str)
                bv = int(bv_str)
                tt = int(tt_str)
                rt = int(rt_str)
                
                if cell_id in self.cells:
                    self.update_cell(cell_id, fd, bv, tt, rt)
                    
            except ValueError:
                continue

    def update_cell(self, cell_id, fd, bv, tt, rt):
        cell = self.cells[cell_id]
        current_state = self.states[cell_id]
        
        delta = bv - fd
        
        if not current_state and delta > tt:
            self.states[cell_id] = True
            cell['frame'].configure(bg=self.active_color)
            cell['label'].configure(bg=self.active_color)
        elif current_state and delta < rt:
            self.states[cell_id] = False
            cell['frame'].configure(bg=self.inactive_color)
            cell['label'].configure(bg=self.inactive_color)
            
        cell['label'].configure(text=f"{cell_id}\nF: {fd}\nB: {bv}\nD: {delta}\nTT: {tt}\nRT: {rt}")


def serial_reader(data_q, cmd_q):
    try:
        # Reduced timeout so we frequently yield to check the command queue
        with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01) as ser:
            print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
            while True:
                # Send any pending commands to the Arduino
                while not cmd_q.empty():
                    cmd = cmd_q.get_nowait()
                    ser.write(cmd)
                    
                line = ser.readline().decode('ascii', errors='ignore')
                if line:
                    data_q.put(line)
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        print("Please check your COM port and ensure the Arduino is not open in the Serial Monitor.")
        sys.exit(1)

if __name__ == "__main__":
    data_queue = queue.Queue()
    cmd_queue = queue.Queue()
    
    ser_thread = threading.Thread(target=serial_reader, args=(data_queue, cmd_queue), daemon=True)
    ser_thread.start()
    
    app = MaimaiVisualizer(data_queue, cmd_queue)
    app.mainloop()