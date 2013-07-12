"""
Basic event loop based on select.
"""

import select
import time

_AFTER = 1
_TIMER = 2
_FILE = 3

class EventLoop():
    def __init__(self):
        self.read_events = {}
        self.read_fds = []
        self.write_events = {}
        self.write_fds = []
        self.timed_events = []
        self.active = True

    def after(self, delay, func, *args):
        """
        Add a one-time event. The event loop will call func(*args) after
        "delay" seconds.

        Returns an object that can be used with cancel() to remove
        the event.
        """
        event = _Event(_AFTER, delay, None, func, args)
        self.timed_events.append(event)

    def timer(self, interval, func, *args):
        """
        Add a recurring event. The event loop will call func(*args) every
        "interval" seconds.

        Returns an object that can be used with cancel() to remove
        the event.
        """
        event = _Event(_TIMER, interval, None, func, args)
        self.timed_events.append(event)

    def cancel(self, event):
        """
        Cancel a timed event previously scheduled with after() or
        timer().
        """
        if event not in self.timed_events:
            return
        self.timed_events.pop(self.timed_events.index(event))

    def readable(self, reader, func, *args):
        """
        Add a readable file descriptor or file type object to the 
        event system. func(*args) will be called any time, a read()
        or recv() operation on the underlying file descriptor will
        not block.

        "reader" can be a native file descriptor as returned by
        os.open(), or any object that defines a fileno() method, as
        long as the resulting descriptor is compatible with
        select.select().

        If "func" is None, an existing callback is removed.
        """
        # ----
        # Determine the actual file descriptor from the reader argument.
        # ----
        try:
            fd = int(reader)
        except Exception as err:
            fd = reader.fileno()

        # ----
        # Remove the event if we're already waiting for it.
        # ----
        if fd in self.read_fds:
            fd_idx = self.read_fds.index(fd)
            self.read_fds.pop(fd_idx)
            del self.read_events[fd]

        # ----
        # Create a (new) event and add it to the system if a callback
        # function is specified.
        # ----
        if func is not None:
            event = _Event(_FILE, None, fd, func, args)
            self.read_events[fd] = event
            self.read_fds.append(fd)

    def writable(self, writer, func, *args):
        """
        Add a writable file descriptor or file type object to the 
        event system. func(*args) will be called any time, a write()
        or send() operation on the underlying file descriptor will
        not block.

        "writer" can be a native file descriptor as returned by
        os.open(), or any object that defines a fileno() method, as
        long as the resulting descriptor is compatible with
        select.select().

        If "func" is None, an existing callback is removed.
        """
        # ----
        # Determine the actual file descriptor from the writer argument.
        # ----
        try:
            fd = int(writer)
        except Exception as err:
            fd = writer.fileno()

        # ----
        # Remove the event if we're already waiting for it.
        # ----
        if fd in self.read_fds:
            fd_idx = self.write_fds.index(fd)
            self.write_fds.pop(fd_idx)
            del self.write_events[fd]

        # ----
        # Create a (new) event and add it to the system if a callback
        # function is specified.
        # ----
        if func is not None:
            event = _Event(_FILE, None, fd, func, args)
            self.write_events[fd] = event
            self.write_fds.append(fd)

    def run_mainloop(self):
        """
        Process events until stop_mainloop() is called.

        An Exception will be thrown if the event system runs completely
        out of events to wait for.
        """
        self.active = True
        while self.active is True:
            self.run_once(None)

    def stop_mainloop(self):
        """
        Cause the mainloop to exit.
        """
        self.active = False

    def run_once(self, timeout):
        """
        Wait for the specified "timeout" in seconds, for any event to
        occur. This method will invoke all callbacks, that are due at
        that time. It will return after "timeout", or if any event
        occured before that. 

        Like run_mainloop(), this will throw an Exception if timeout is
        None and there are no possible events left to wait for.

        Returns the number of event callbacks performed or zero if the
        timeout elapsed.
        """
        # ----
        # Adjust the timeout to any timed event, that is due earlier.
        # ----
        if len(self.timed_events) > 0:
            events = sorted(self.timed_events,
                    key = lambda event: event.ev_due_at)
            next_timeout = events[0].ev_due_at - time.time()
            if timeout is None or next_timeout < timeout:
                timeout = next_timeout

        # ----
        # See if we have any file based events.
        # ----
        have_fds = len(self.read_fds) > 0 or len(self.write_fds) > 0

        # ----
        # Make sure we are not literally going to wait "forever"
        # ----
        if timeout is None and have_fds is False:
            raise Exception('nothing left to wait for')
        
        # ----
        # Wait for something to happen.
        # ----
        num_callbacks = 0
        if have_fds is True:
            if timeout is not True and timeout < 0.0:
                timeout = 0.0
            ready_read, ready_write, ready_err = select.select(
                    self.read_fds, self.write_fds, [], timeout)
        else:
            ready_read = []
            ready_write = []
            ready_err = []
            if timeout > 0.0:
                time.sleep(timeout)

        # ----
        # Perform callbacks for all readable files.
        # ----
        for fd in ready_read:
            self.read_events[fd].invoke()
            num_callbacks += 1
            
        # ----
        # Perform callbacks for all writable files.
        # ----
        for fd in ready_write:
            self.write_events[fd].invoke()
            num_callbacks += 1
            
        # ----
        # Perform callbacks for all timed events.
        # ----
        while len(self.timed_events) > 0:
            # ----
            # Need to resort the events in due-time order. Previous
            # event handlers could have changed the list.
            # ----
            events = sorted(self.timed_events,
                    key = lambda event: event.ev_due_at)

            # ----
            # Exit loop if no more events due at this time.
            # ----
            if events[0].ev_due_at > time.time():
                break

            # ----
            # Get the first due event from the list. For AFTER type events,
            # remove it from the list. For TIMER type, reschedule it.
            # ----
            event = events[0]
            if event.ev_type == _AFTER:
                self.timed_events = events[1:]
            else:
                event.ev_due_at += event.ev_interval

            # ----
            # Invoke the callback function.
            # ----
            event.invoke()
            num_callbacks += 1
        
        # ----
        # Return the number of events, that occurred.
        # ----
        return num_callbacks

class _Event():
    """
    Internal class representing one event/callback. This is used
    as the "event_id" to cancel timed events. But the object should
    be opaque to the outside world.
    """
    def __init__(self, ev_type, ev_interval, ev_file, ev_func, ev_args):
        self.ev_type = ev_type
        self.ev_interval = ev_interval
        if ev_type == _FILE:
            self.ev_file = ev_file
        else:
            self.ev_due_at = time.time() + ev_interval
        self.ev_func = ev_func
        self.ev_args = ev_args

    def invoke(self):
        self.ev_func(self, *self.ev_args)

    def interval(self, new_interval = None):
        result = self.ev_interval
        if new_interval is not None:
            self.ev_interval = new_interval
        return result
