#!/usr/bin/python

import nest

mmip = nest.Create ('music_message_in_proxy')
nest.SetStatus (mmip, {'port_name' : 'in', 'max_buffered' : 10})

# Simulate and get message data with a granularity of 10 ms:
time = 0
while time < 1000:
    nest.Simulate (10)
    data = nest.GetStatus(mmip, 'data')
    print data
    nest.SetStatus(mmip, {'n_messages' : 0}) # clear input buffer
    time += 10
