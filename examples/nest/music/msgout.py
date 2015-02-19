#!/usr/bin/python

import nest

mmop = nest.Create ('music_message_out_proxy')
nest.SetStatus (mmop, {'port_name' : 'out'})

time = 0
nest.Simulate (1) # enter MUSIC runtime phase (temporary kludge)
while time < 1000:
    data = nest.SetStatus(mmop,
                          {'messages' : ['hello'], 'message_times' : [time] })
    nest.Simulate (10)
    time += 10
