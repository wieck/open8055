#!/usr/bin/env python

import Tkinter as tk
import ttk
import sys
import open8055

def main(argv):
    root = tk.Tk()
    # root.geometry('400x300')
    root.title('Open8055 Demo')

    demo = Open8055Demo(root)
    demo.pack(side=tk.TOP, fill=tk.BOTH, expand=True, pady=8)

    root.mainloop()

    return 0

# ----------------------------------------------------------------------
# Open8055Demo
#
#   Class implementing the demo application.
# ----------------------------------------------------------------------
class Open8055Demo(tk.Frame):
    def __init__(self, parent):
        tk.Frame.__init__(self, parent, borderwidth=0, relief=tk.FLAT)

        parent.wm_protocol('WM_DELETE_WINDOW', self.cancel)
        self.parent = parent
        self.destination = tk.StringVar(value='open8055://nobody@localhost/card0')
        self.conn = None
        self.password = tk.StringVar(value=None)
        self.message = tk.StringVar(value='Not connected')

        self.I1_mode = open8055.MODE_INPUT
        self.I1_checked = tk.BooleanVar(value=False)

        f0 = tk.Frame(self, borderwidth=2, relief=tk.FLAT)
        f0.pack(side=tk.TOP, fill=tk.X, pady=8)

        # ----
        # The Host, Username and Password labels on the left
        # ----
        l0 = tk.Label(f0, text='Card destination:', borderwidth=2, relief=tk.FLAT)
        l0.grid(column=0, row=0, sticky='e', padx=2, pady=2)

        l1 = tk.Label(f0, text='Password:', borderwidth=2, relief=tk.FLAT)
        l1.grid(column=0, row=1, sticky='e', padx=2, pady=2)

        # ----
        # The corresponding entries to the right. The destination
        # entry initially has the focus and all content selected.
        # ----
        self.e0 = tk.Entry(f0, width=30, borderwidth=2, relief=tk.SUNKEN,
                textvariable=self.destination)
        self.e0.grid(column=1, row=0, sticky='we', padx=2, pady=2)
        self.e0.selection_range(0, 'end')
        self.e0.focus()
        self.e0.bind('<Return>', self.connect)
        self.e0.bind('<Escape>', self.cancel)

        self.e1 = tk.Entry(f0, width=30, borderwidth=2, relief=tk.SUNKEN,
                textvariable=self.password, show='*')
        self.e1.grid(column=1, row=1, sticky='we', padx=2, pady=2)
        self.e1.bind('<Return>', self.connect)
        self.e1.bind('<Escape>', self.cancel)

        # ----
        # The Connect and Disconnect buttons.
        # ----
        self.b0 = tk.Button(f0, text="Connect", borderwidth=2, relief=tk.RAISED,
                width=10, command=self.connect)
        self.b0.grid(column=2, row=0, padx=2, pady=2)
        self.b0.bind('<Return>', self.connect)
        self.b0.bind('<Escape>', self.cancel)

        self.b1 = tk.Button(f0, text="Disconnect", borderwidth=2, relief=tk.RAISED,
                width=10, command=self.disconnect, state=tk.DISABLED)
        self.b1.grid(column=2, row=1, padx=2, pady=2)
        self.b1.bind('<Return>', self.disconnect)

        # ----
        # The error message field and the Card Reset button.
        # ----
        l2 = tk.Label(f0, textvariable=self.message, width=50, borderwidth=2,
                relief=tk.FLAT, anchor='w')
        l2.grid(column=3, row=0, sticky='w', padx=2, pady=2)

        self.b2 = tk.Button(f0, text='Reset Card', borderwidth=2, relief=tk.RAISED,
                width=10, command=self.reset_card, state=tk.DISABLED)
        self.b2.grid(column=3, row=1, sticky='w', padx=2, pady=2)

        # ----
        # The bottom frame for all the control elements.
        # ----
        f1 = tk.Frame(self, borderwidth=2, relief=tk.FLAT)
        f1.pack(side=tk.TOP, fill=tk.X, pady=8)

        la1 = tk.Label(f1, text='A1:', borderwidth=2, relief=tk.FLAT, anchor='w')
        la1.grid(column=0, row=0, sticky='w')
        la2 = tk.Label(f1, text='A2:', borderwidth=2, relief=tk.FLAT, anchor='w')
        la2.grid(column=0, row=1, sticky='w')

        li1 = tk.Label(f1, text='I1:', borderwidth=2, relief=tk.FLAT, anchor='w')
        li1.grid(column=0, row=2, sticky='w')
        li2 = tk.Label(f1, text='I2:', borderwidth=2, relief=tk.FLAT, anchor='w')
        li2.grid(column=0, row=3, sticky='w')
        li3 = tk.Label(f1, text='I3:', borderwidth=2, relief=tk.FLAT, anchor='w')
        li3.grid(column=0, row=4, sticky='w')
        li4 = tk.Label(f1, text='I4:', borderwidth=2, relief=tk.FLAT, anchor='w')
        li4.grid(column=0, row=5, sticky='w')
        li5 = tk.Label(f1, text='I5:', borderwidth=2, relief=tk.FLAT, anchor='w')
        li5.grid(column=0, row=6, sticky='w')
        
        lpwm1 = tk.Label(f1, text='PWM1:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lpwm1.grid(column=0, row=7, sticky='w')
        lpwm2 = tk.Label(f1, text='PWM2:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lpwm2.grid(column=0, row=8, sticky='w')

        lo1 = tk.Label(f1, text='O1:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo1.grid(column=0, row=9, sticky='w')
        lo2 = tk.Label(f1, text='O2:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo2.grid(column=0, row=10, sticky='w')
        lo3 = tk.Label(f1, text='O3:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo3.grid(column=0, row=11, sticky='w')
        lo4 = tk.Label(f1, text='O4:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo4.grid(column=0, row=12, sticky='w')
        lo5 = tk.Label(f1, text='O5:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo5.grid(column=0, row=13, sticky='w')
        lo6 = tk.Label(f1, text='O6:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo6.grid(column=0, row=14, sticky='w')
        lo7 = tk.Label(f1, text='O7:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo7.grid(column=0, row=15, sticky='w')
        lo8 = tk.Label(f1, text='O8:', borderwidth=2, relief=tk.FLAT, anchor='w')
        lo8.grid(column=0, row=16, sticky='w')

        self.adc1 = Open8055Adc(f1, self, 0)
        self.adc1.grid(column=1, row=0, sticky='w')
        self.adc2 = Open8055Adc(f1, self, 1)
        self.adc2.grid(column=1, row=1, sticky='w')

        self.in1 = Open8055In(f1, self, 0)
        self.in1.grid(column=1, row=2, sticky='w')
        self.in2 = Open8055In(f1, self, 1)
        self.in2.grid(column=1, row=3, sticky='w')
        self.in3 = Open8055In(f1, self, 2)
        self.in3.grid(column=1, row=4, sticky='w')
        self.in4 = Open8055In(f1, self, 3)
        self.in4.grid(column=1, row=5, sticky='w')
        self.in5 = Open8055In(f1, self, 4)
        self.in5.grid(column=1, row=6, sticky='w')

    def connect(self, event=None):
        self.e0.configure(state=tk.DISABLED)
        self.e1.configure(state=tk.DISABLED)
        self.b0.configure(state=tk.DISABLED)

        try:
            conninfo = open8055.conninfo(self.destination.get(),
                    self.password.get())
            self.conn = open8055.open(conninfo)
        except Exception as err:
            self.message.set(str(err))
            self.e0.configure(state=tk.NORMAL)
            self.e1.configure(state=tk.NORMAL)
            self.b0.configure(state=tk.NORMAL)
            return

        self.b1.configure(state=tk.NORMAL)
        self.b2.configure(state=tk.NORMAL)

        self.b1.focus()

        self.conn_update()
        self.parent.tk.createfilehandler(self.conn, tk.tkinter.READABLE, 
                self.conn_readable)

    def disconnect(self, event=None):
        self.parent.tk.deletefilehandler(self.conn)
        self.conn.close()
        self.conn = None
        self.message.set('Connection closed')

        self.e0.configure(state=tk.NORMAL)
        self.e1.configure(state=tk.NORMAL)
        self.b0.configure(state=tk.NORMAL)
        self.b1.configure(state=tk.DISABLED)
        self.b2.configure(state=tk.DISABLED)

        self.e0.selection_range(0, 'end')
        self.e0.focus()

        self.conn_update()

    def cancel(self, event=None):
        if self.conn is not None:
            self.disconnect()
        self.parent.destroy()

    def reset_card(self, event=None):
        self.conn.reset()

    def conn_readable(self, conn, mask):
        try:
            self.conn.poll()
            self.conn_update()
        except Exception as err:
            self.disconnect()
            self.message.set(str(err))

    def conn_update(self):
        self.adc1.update()
        self.adc2.update()
        self.in1.update()
        self.in2.update()
        self.in3.update()
        self.in4.update()
        self.in5.update()

class Open8055In(tk.Frame):
    def __init__(self, parent, main, port):
        tk.Frame.__init__(self, parent, borderwidth=0)

        self.parent = parent
        self.main = main
        self.port = port
        self.active = tk.BooleanVar(value=False)
        self.counter = tk.StringVar(value='')
        self.curmode = tk.StringVar(value='')
        self.curdebounce = tk.StringVar(value='')
        self.lastdebounce = -1.0

        self.cb = tk.Checkbutton(self, borderwidth=0, relief=tk.SUNKEN,
                variable=self.active, state=tk.DISABLED,
                disabledforeground='black')
        self.cb.pack(side=tk.LEFT, padx=2, pady=2)
        self.mode = ttk.Combobox(self, 
                textvariable=self.curmode, width=15, state=tk.DISABLED,
                values=['Mode Normal', 'Mode Frequency'])
        self.curmode.trace('w', self.mode_change)
        self.mode.pack(side=tk.LEFT, padx=4, pady=2)
        l = tk.Label(self, borderwidth=2, relief=tk.FLAT, text='Counter:')
        l.pack(side=tk.LEFT, padx=4, pady=2)
        l = tk.Label(self, borderwidth=2, relief=tk.SUNKEN, width=8,
                textvariable=self.counter, anchor='e')
        l.pack(side=tk.LEFT, padx=4, pady=2)
        self.reset = tk.Button(self, borderwidth=2, relief=tk.RAISED,
                text='Reset', command=self.reset_counter, state=tk.DISABLED)
        self.reset.pack(side=tk.LEFT, padx=4, pady=2)
        l = tk.Label(self, borderwidth=2, relief=tk.FLAT, text='Debounce (ms):')
        l.pack(side=tk.LEFT, padx=4, pady=2)
        self.debounce = tk.Entry(self, textvariable=self.curdebounce, 
                state=tk.DISABLED)
        self.debounce.pack(side=tk.LEFT, padx=4, pady=2)
        self.debset = tk.Button(self, borderwidth=2, relief=tk.RAISED,
                text='Set', command=self.set_debounce, state=tk.DISABLED)
        self.debset.pack(side=tk.LEFT, padx=4, pady=2)
        self.debounce.bind('<Return>', self.set_debounce)

    def update(self):
        if self.main.conn is not None:
            self.active.set(self.main.conn.get_input(self.port))
            self.mode.configure(state=tk.NORMAL)
            if self.main.conn.get_input_mode(self.port) == open8055.MODE_INPUT and self.curmode.get() != 'Mode Normal':
                self.curmode.set('Mode Normal')
            elif self.main.conn.get_input_mode(self.port) == open8055.MODE_FREQUENCY and self.curmode.get() != 'Mode Frequency':
                self.curmode.set('Mode Frequency')
            self.counter.set(str(self.main.conn.get_counter(self.port)))
            self.reset.configure(state=tk.NORMAL)
            self.debounce.configure(state=tk.NORMAL)
            self.debset.configure(state=tk.NORMAL)
            deb = self.main.conn.get_debounce(self.port) * 1000.0
            if deb != self.lastdebounce:
                self.curdebounce.set('{0:.1f}'.format(deb))
                self.lastdebounce = deb
        else:
            self.active.set(False)
            self.mode.configure(state=tk.DISABLED)
            self.curmode.set('')
            self.counter.set('')
            self.reset.configure(state=tk.DISABLED)
            self.debounce.configure(state=tk.DISABLED)
            self.debset.configure(state=tk.DISABLED)
            self.curdebounce.set('')
            self.lastdebounce = -1.0

    def mode_change(self, var, val, action):
        if self.curmode.get() == 'Mode Normal':
            self.main.conn.set_input_mode(self.port, open8055.MODE_INPUT)
        elif self.curmode.get() == 'Mode Frequency':
            self.main.conn.set_input_mode(self.port, open8055.MODE_FREQUENCY)

    def reset_counter(self):
        self.main.conn.reset_counter(self.port)

    def set_debounce(self, dummy=None):
        try:
            deb = '{0:.1}'.format(float(self.curdebounce.get()))
        except Exception:
            deb = '{0:.1}'.format(self.lastdebounce)
        if float(deb) > 5000.0:
            deb = '5000.0'
        self.main.conn.set_debounce(self.port, float(deb) / 1000.0)

class Open8055Adc(tk.Frame):
    def __init__(self, parent, main, port):
        tk.Frame.__init__(self, parent, borderwidth=0)

        self.parent = parent
        self.main = main
        self.port = port
        self.value = tk.StringVar(value='')
        self.curmode = tk.IntVar(value=None)

        self.val = tk.Label(self, borderwidth=2, relief=tk.SUNKEN, 
                width=8, textvariable=self.value, anchor=tk.W)
        self.val.pack(side=tk.LEFT, padx=2, pady=2)

        self.can = tk.Canvas(self, borderwidth=1, relief=tk.SUNKEN,
                width=512, height=1, highlightthickness=0)
        self.can.pack(side=tk.LEFT, fill=tk.Y, padx=8, pady=2)
        self.can.create_rectangle([-1, -1, -1, -1], fill='blue2', outline='',
                tags='bar')
        
        self.adc10 = tk.Radiobutton(self, borderwidth=0, relief=tk.FLAT,
                text='10 Bit', variable=self.curmode, value=open8055.MODE_ADC10,
                state=tk.DISABLED, command=self.change_bits)
        self.adc10.pack(side=tk.LEFT, padx=4, pady=2)
        self.adc9 = tk.Radiobutton(self, borderwidth=0, relief=tk.FLAT,
                text='9 Bit', variable=self.curmode, value=open8055.MODE_ADC9,
                state=tk.DISABLED, command=self.change_bits)
        self.adc9.pack(side=tk.LEFT, padx=4, pady=2)
        self.adc8 = tk.Radiobutton(self, borderwidth=0, relief=tk.FLAT,
                text='8 Bit', variable=self.curmode, value=open8055.MODE_ADC8,
                state=tk.DISABLED, command=self.change_bits)
        self.adc8.pack(side=tk.LEFT, padx=4, pady=2)

    def update(self):
        if self.main.conn is not None:
            self.adc10.configure(state=tk.NORMAL)
            self.adc9.configure(state=tk.NORMAL)
            self.adc8.configure(state=tk.NORMAL)
            self.value.set('{0:7.6f}'.format(
                    self.main.conn.get_adc(self.port)))
            self.can.coords('bar', 0, 0, float(self.value.get()) * 512, 50)
            self.curmode.set(self.main.conn.get_adc_mode(self.port))
        else:
            self.adc10.configure(state=tk.DISABLED)
            self.adc9.configure(state=tk.DISABLED)
            self.adc8.configure(state=tk.DISABLED)
            self.value.set('')
            self.can.coords('bar', 0, 0, 0, 0)
            self.curmode.set(None)

    def change_bits(self):
        self.main.conn.set_adc_mode(self.port, self.curmode.get())

# ----------------------------------------------------------------------
# Call main()
# ----------------------------------------------------------------------
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
