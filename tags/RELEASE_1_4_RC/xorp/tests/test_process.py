#!/usr/bin/env python

# Copyright (c) 2001-2007 International Computer Science Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software")
# to deal in the Software without restriction, subject to the conditions
# listed in the XORP LICENSE file. These conditions include: you must
# preserve this copyright notice, and you cannot mention the copyright
# holders in advertising related to the Software without their permission.
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
# notice is a summary of the XORP LICENSE file; the license in that file is
# legally binding.

# $XORP: xorp/tests/test_process.py,v 1.1 2006/04/07 05:13:08 atanu Exp $

import thread,threading,time,sys,os,popen2

class Process(threading.Thread):
    """
    Start a process in a separate thread
    """

    def __init__(self, command=""):
        threading.Thread.__init__(self)
        self._status = "INIT"
        self.command = command
        self.lock = thread.allocate_lock()
        
    def run(self):
        self.lock.acquire()
        print "command:", self.command
        self.process = popen2.Popen4("exec " + self.command)
        print "PID:", self.process.pid
        self._status = "RUNNING"
        while 1:
            o = self.process.fromchild.read(1)
            if not o:
                break
            os.write(1, o)
        self._status = "TERMINATED"
        print "exiting:", self.command
        self.lock.release()

    def status(self):
        return self._status

    def terminate(self):
        """
        Terminate this process
        """

        print "sending kill to", self.command, self.process.pid
        if self._status == "RUNNING":
            os.kill(self.process.pid, 9)
            self.lock.acquire()
            self.lock.release()
        else:
            print self.command, "not running"


# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
