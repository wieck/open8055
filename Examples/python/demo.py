#!/usr/bin/env python

from Tkinter import *
import sys, signal
import open8055

signal.signal(signal.SIGINT, signal.SIG_DFL)

def main(argv):
    root = Tk()
    #root.geometry('400x300')
    root.title('Open8055 Demo')

    login_screen = LoginScreen(root)

    root.mainloop()

    return 0

# ----------------------------------------------------------------------
# LoginScreen
#
#   Class implementing the login screen.
# ----------------------------------------------------------------------
class LoginScreen(Frame):
    def __init__(self, root):
        Frame.__init__(self, root, borderwidth=4, relief=FLAT)

        self.root = root
        self.hostname = StringVar(value='localhost:8055')
        self.username = StringVar(value='nobody')
        self.password = StringVar(value=None)

        self.setup()

    def setup(self):
        self.pack(side=TOP, fill=BOTH, expand=True)

        # ----
        # The Host, Username and Password labels on the left
        # ----
        l0 = Label(self, text='Host:', borderwidth=2, relief=FLAT)
        l0.grid(column=0, row=0, sticky='w', padx=2, pady=2)

        l1 = Label(self, text='Username:', borderwidth=2, relief=FLAT)
        l1.grid(column=0, row=1, sticky='w', padx=2, pady=2)

        l2 = Label(self, text='Password:', borderwidth=2, relief=FLAT)
        l2.grid(column=0, row=2, sticky='w', padx=2, pady=2)

        # ----
        # The corresponding entries to the right. The host
        # entry initially has the focus and all content selected.
        # ----
        self.e0 = Entry(self, width=50, borderwidth=2, relief=SUNKEN,
                textvariable=self.hostname)
        self.e0.grid(column=1, row=0, sticky='we', padx=2, pady=2)
        self.e0.selection_range(0, 'end')
        self.e0.focus()

        self.e1 = Entry(self, width=50, borderwidth=2, relief=SUNKEN,
                textvariable=self.username)
        self.e1.grid(column=1, row=1, sticky='we', padx=2, pady=2)

        self.e2 = Entry(self, width=50, borderwidth=2, relief=SUNKEN,
                textvariable=self.password, show='*')
        self.e2.grid(column=1, row=2, sticky='we', padx=2, pady=2)

        # ----
        # The LOGIN and EXIT buttons at the bottom.
        # ----
        self.bf0 = Frame(self, borderwidth=0)
        self.bf0.grid(column=0, row=3, columnspan=2)

        b0 = Button(self.bf0, text="LOGIN", borderwidth=2, relief=RAISED,
                width=10, command=self.login)
        b0.pack(side=LEFT)
        b0.bind('<Return>', self.login)
        b0.bind('<Escape>', self.cancel)

        b1 = Button(self.bf0, text="EXIT", borderwidth=2, relief=RAISED,
                width=10, command=self.cancel)
        b1.pack(side=LEFT)
        b1.bind('<Return>', self.cancel)
        b1.bind('<Escape>', self.cancel)

        # ----
        # The LOGOUT and EXIT buttons, displayed after login.
        # ----
        self.bf1 = Frame(self, borderwidth=0)

        self.blogout = Button(self.bf1, text="DISCONNECT", borderwidth=2, 
                relief=RAISED, width=10, command=self.logout)
        self.blogout.pack(side=LEFT)
        self.blogout.bind('<Return>', self.logout)
        self.blogout.bind('<Escape>', self.cancel)

        b1 = Button(self.bf1, text="EXIT", borderwidth=2, relief=RAISED,
                width=10, command=self.cancel)
        b1.pack(side=LEFT)
        b1.bind('<Return>', self.cancel)
        b1.bind('<Escape>', self.cancel)

        # ----
        # Hitting <Return> in any of the entries calls login().
        # ----
        self.e0.bind('<Return>', self.login)
        self.e1.bind('<Return>', self.login)
        self.e2.bind('<Return>', self.login)

        # ----
        # Hitting <Escape> in any of the entries calls cancel().
        # ----
        self.e0.bind('<Escape>', self.cancel)
        self.e1.bind('<Escape>', self.cancel)
        self.e2.bind('<Escape>', self.cancel)

    def login(self, event=None):
        print 'login()'
        print 'hostname:', self.hostname.get()
        print 'username:', self.username.get()
        print 'password:', self.password.get()

        self.e0.configure(state='disabled')
        self.e1.configure(state='disabled')
        self.e2.configure(state='disabled')

        self.bf0.grid_forget()
        self.bf1.grid(column=0, row=3, columnspan=2)
        self.blogout.focus()

    def logout(self, event=None):
        print 'logout()'

        self.e0.configure(state='normal')
        self.e1.configure(state='normal')
        self.e2.configure(state='normal')

        self.e0.selection_range(0, 'end')
        self.e0.focus()

        self.bf1.grid_forget()
        self.bf0.grid(column=0, row=3, columnspan=2)

    def cancel(self, event=None):
        self.root.destroy()


# ----------------------------------------------------------------------
# Call main()
# ----------------------------------------------------------------------
if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
