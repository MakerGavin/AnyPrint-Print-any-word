import socket
import threading
import time
import win32con
import pyautogui
import win32api
import pyperclip
import tkinter as tk
from tkinter import messagebox, simpledialog

# TCP server configuration
TCP_IP = '10.0.1.61'
TCP_PORT = 80

class SettingsWindow:
    def __init__(self, parent):
        self.parent = parent
        self.window = tk.Toplevel(parent)
        self.window.title("Cloud Printer Settings")
        self.window.geometry("300x150")

        self.server_label = tk.Label(self.window, text="Enter Cloud Printer IP address:")
        self.server_label.pack()

        self.server_entry = tk.Entry(self.window)
        self.server_entry.pack()

        self.port_label = tk.Label(self.window, text="Enter Cloud Printer Port number:")
        self.port_label.pack()

        self.port_entry = tk.Entry(self.window)
        self.port_entry.pack()
        self.port_entry.pack()

        self.button = tk.Button(self.window, text="Confirm", command=self.update_settings)
        self.button.pack()

    def update_settings(self):
        new_server = self.server_entry.get()
        new_port = self.port_entry.get()

        if new_server:
            global TCP_IP
            TCP_IP = new_server
        
        if new_port:
            global TCP_PORT
            TCP_PORT = int(new_port)
        
        self.window.destroy()  # Close the settings window
            
class MouseThread(threading.Thread):
    def __init__(self):
        super().__init__()
        self.running = True
        self.left_click_start_time = 0
        self.right_click_start_time = 0
        self.left_hold = False
        self.right_click_pressed = False
        self.left_click_pressed = False
        self.selected_text = ""

    def run(self):
        while self.running:
            self.check_left_click()
            self.check_right_click()
            time.sleep(0.1)

    def check_right_click(self):
        if self.left_hold:
            if win32api.GetAsyncKeyState(win32con.VK_RBUTTON) & 0x8000:
                if not self.right_click_pressed:
                    self.right_click_start_time = time.time()
                    self.right_click_pressed = True
            else:
                if self.right_click_pressed and time.time() - self.right_click_start_time >= 0.5:
                    self.show_send_confirmation(self.selected_text)
                    self.left_hold = False
                self.right_click_pressed = False                

    def check_left_click(self):
        if win32api.GetAsyncKeyState(win32con.VK_LBUTTON) & 0x8000:
            if not self.left_click_pressed:
                self.left_click_start_time = time.time()
                self.left_click_pressed = True
        else:
            if self.left_click_pressed and time.time() - self.left_click_start_time >= 1:
                self.left_hold = True
                pyautogui.hotkey('ctrl','c')  # Simulate Ctrl+C to copy selected text
                time.sleep(0.1) 
                self.selected_text = pyperclip.paste()  # Get text from clipboard
            self.left_click_pressed = False

    def send_text(self, text):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((TCP_IP, TCP_PORT))
                # s.sendall("TEXT".encode())
                print("TEXT"+text)
                s.sendall(("TEXT"+text).encode())

                # 数据传输完毕后关闭连接
                s.shutdown(socket.SHUT_RDWR)
                self.selected_text = ""
        except Exception as e:
            print("Error sending text:", e)

    def show_send_confirmation(self, selected_text):
        root = tk.Tk()
        root.withdraw()

        answer = messagebox.askyesno("Cloud Printer Confirmation", f"Do you want to Print the following text\n\nCloud Printer: {TCP_IP}：{TCP_PORT}\n\n{selected_text}")
        if answer:
            self.send_text(selected_text)
        else:
            settings_window = SettingsWindow(root)
            root.wait_window(settings_window.window)

if __name__ == "__main__":
    mouse_thread = MouseThread()
    mouse_thread.start()

    try:
        while True:
            pass
    except KeyboardInterrupt:
        mouse_thread.running = False
        mouse_thread.join()
