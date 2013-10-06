#!/usr/bin/env python

from Tkinter import *
import sys
import open8055

def main(argv):
    root = Tk()
    # root.geometry('400x300')
    root.title('Open8055 Demo')

    demo = Open8055Demo(root)
    demo.pack(side=TOP, fill=BOTH, expand=True, pady=8)

    root.mainloop()

    return 0

# ----------------------------------------------------------------------
# Open8055Demo
#
#   Class implementing the demo application.
# ----------------------------------------------------------------------
class Open8055Demo(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, borderwidth=0, relief=FLAT)

        self.parent = parent
        self.destination = StringVar(value='open8055://nobody@localhost/card0')
        self.password = StringVar(value=None)
        self.message = StringVar(value='Not connected')

        self.I1_mode = open8055.MODE_INPUT
        self.I1_checked = BooleanVar(value=False)

        f0 = Frame(self, borderwidth=2, relief=FLAT)
        f0.pack(side=TOP, fill=X, pady=8)

        # ----
        # The Host, Username and Password labels on the left
        # ----
        l0 = Label(f0, text='Card destination:', borderwidth=2, relief=FLAT)
        l0.grid(column=0, row=0, sticky='e', padx=2, pady=2)

        l1 = Label(f0, text='Password:', borderwidth=2, relief=FLAT)
        l1.grid(column=0, row=1, sticky='e', padx=2, pady=2)

        # ----
        # The corresponding entries to the right. The destination
        # entry initially has the focus and all content selected.
        # ----
        self.e0 = Entry(f0, width=30, borderwidth=2, relief=SUNKEN,
                textvariable=self.destination)
        self.e0.grid(column=1, row=0, sticky='we', padx=2, pady=2)
        self.e0.selection_range(0, 'end')
        self.e0.focus()
        self.e0.bind('<Return>', self.connect)
        self.e0.bind('<Escape>', self.cancel)

        self.e1 = Entry(f0, width=30, borderwidth=2, relief=SUNKEN,
                textvariable=self.password, show='*')
        self.e1.grid(column=1, row=1, sticky='we', padx=2, pady=2)
        self.e1.bind('<Return>', self.connect)
        self.e1.bind('<Escape>', self.cancel)

        # ----
        # The Connect and Disconnect buttons.
        # ----
        self.b0 = Button(f0, text="Connect", borderwidth=2, relief=RAISED,
                width=10, command=self.connect)
        self.b0.grid(column=2, row=0, padx=2, pady=2)
        self.b0.bind('<Return>', self.connect)
        self.b0.bind('<Escape>', self.cancel)

        self.b1 = Button(f0, text="Disconnect", borderwidth=2, relief=RAISED,
                width=10, command=self.disconnect, state=DISABLED)
        self.b1.grid(column=2, row=1, padx=2, pady=2)
        self.b1.bind('<Return>', self.disconnect)

        # ----
        # The error message field and the Card Reset button.
        # ----
        l2 = Label(f0, textvariable=self.message, width=50, borderwidth=2,
                relief=FLAT, anchor='w')
        l2.grid(column=3, row=0, sticky='w', padx=2, pady=2)

        self.b2 = Button(f0, text='Reset Card', borderwidth=2, relief=RAISED,
                width=10, command=self.reset_card, state=DISABLED)
        self.b2.grid(column=3, row=1, sticky='w', padx=2, pady=2)

        # ----
        # The bottom frame for all the control elements.
        # ----
        f1 = Frame(self, borderwidth=2, relief=FLAT)
        f1.pack(side=TOP, fill=X, pady=8)

        la1 = Label(f1, text='A1:', borderwidth=2, relief=FLAT, anchor='w')
        la1.grid(column=0, row=0, sticky='w')
        la2 = Label(f1, text='A2:', borderwidth=2, relief=FLAT, anchor='w')
        la2.grid(column=0, row=1, sticky='w')

        li1 = Label(f1, text='I1:', borderwidth=2, relief=FLAT, anchor='w')
        li1.grid(column=0, row=2, sticky='w')
        li2 = Label(f1, text='I2:', borderwidth=2, relief=FLAT, anchor='w')
        li2.grid(column=0, row=3, sticky='w')
        li3 = Label(f1, text='I3:', borderwidth=2, relief=FLAT, anchor='w')
        li3.grid(column=0, row=4, sticky='w')
        li4 = Label(f1, text='I4:', borderwidth=2, relief=FLAT, anchor='w')
        li4.grid(column=0, row=5, sticky='w')
        li5 = Label(f1, text='I5:', borderwidth=2, relief=FLAT, anchor='w')
        li5.grid(column=0, row=6, sticky='w')
        
        lpwm1 = Label(f1, text='PWM1:', borderwidth=2, relief=FLAT, anchor='w')
        lpwm1.grid(column=0, row=7, sticky='w')
        lpwm2 = Label(f1, text='PWM2:', borderwidth=2, relief=FLAT, anchor='w')
        lpwm2.grid(column=0, row=8, sticky='w')

        lo1 = Label(f1, text='O1:', borderwidth=2, relief=FLAT, anchor='w')
        lo1.grid(column=0, row=9, sticky='w')
        lo2 = Label(f1, text='O2:', borderwidth=2, relief=FLAT, anchor='w')
        lo2.grid(column=0, row=10, sticky='w')
        lo3 = Label(f1, text='O3:', borderwidth=2, relief=FLAT, anchor='w')
        lo3.grid(column=0, row=11, sticky='w')
        lo4 = Label(f1, text='O4:', borderwidth=2, relief=FLAT, anchor='w')
        lo4.grid(column=0, row=12, sticky='w')
        lo5 = Label(f1, text='O5:', borderwidth=2, relief=FLAT, anchor='w')
        lo5.grid(column=0, row=13, sticky='w')
        lo6 = Label(f1, text='O6:', borderwidth=2, relief=FLAT, anchor='w')
        lo6.grid(column=0, row=14, sticky='w')
        lo7 = Label(f1, text='O7:', borderwidth=2, relief=FLAT, anchor='w')
        lo7.grid(column=0, row=15, sticky='w')
        lo8 = Label(f1, text='O8:', borderwidth=2, relief=FLAT, anchor='w')
        lo8.grid(column=0, row=16, sticky='w')

        self.input_ctrl = {}
        self.new_input_ctrl(f1, 'I1').grid(column=1, row=2, sticky='w')

    def new_input_ctrl(self, parent, name):
        f = Frame(parent, borderwidth=0)
        var = BooleanVar(value=False)
        cb = Checkbutton(f, borderwidth=2, relief=FLAT,
                state=DISABLED, variable=var)
        cb.pack(side=LEFT)

        self.input_ctrl[name] = {'var': var, 'cb': cb}
        return f

    def connect(self, event=None):
        print 'connect()'
        print 'destination:', self.destination.get()
        print 'password:', self.password.get()

        self.e0.configure(state=DISABLED)
        self.e1.configure(state=DISABLED)
        self.b0.configure(state=DISABLED)

        try:
            conninfo = open8055.conninfo(self.destination.get(),
                    self.password.get())
            self.conn = open8055.open(conninfo)
        except Exception as err:
            self.message.set(str(err))
            self.e0.configure(state=NORMAL)
            self.e1.configure(state=NORMAL)
            self.b0.configure(state=NORMAL)
            return

        self.b1.configure(state=NORMAL)
        self.b2.configure(state=NORMAL)

        self.b1.focus()

        self.conn_update()
        self.parent.tk.createfilehandler(self.conn, tkinter.READABLE, 
                self.conn_readable)

    def disconnect(self, event=None):
        self.parent.tk.deletefilehandler(self.conn)
        self.conn.close()
        del(self.conn)
        self.message.set('Connection closed')

        self.e0.configure(state=NORMAL)
        self.e1.configure(state=NORMAL)
        self.b0.configure(state=NORMAL)
        self.b1.configure(state=DISABLED)
        self.b2.configure(state=DISABLED)

        self.e0.selection_range(0, 'end')
        self.e0.focus()

    def cancel(self, event=None):
        self.root.destroy()

    def reset_card(self, event=None):
        pass

    def conn_readable(self, conn, mask):
        try:
            self.conn.poll()
            self.conn_update()
        except Exception as err:
            self.disconnect()
            self.message.set(str(err))

    def conn_update(self):
        self.input_ctrl['I1']['var'].set(self.conn.get_input(0))
        

# ----------------------------------------------------------------------
# Call main()
# ----------------------------------------------------------------------
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
