import sys
from datetime import datetime
import re
import json
from collections import OrderedDict # needed for json to preserve key order...

# Usage: python3 fix-logs.py < ~/shower.log > ~/shower.fix.log

# This script is not idempotent and will throw an exception if e.g. the date is already in the right format!
# Adjust which fixes should be done:

fix_date = True
# Date should be e.g. 2020-01-16 instead of Jan 16.
# Logs start 2019-03-12, and now it's 2020-01-22. -> If month is Jan set year to 2020, otherwise 2019.

fix_json = True
# All json keys and strings should be quoted with "" so that telegraf can parse them. Before 2019-04-12 keys were unquoted and strings quoted with ''.
# Rename fields before 2019-04-12 ('time' used by telegraf, but field is just Arduino's millis() (since boot)): time -> millis, startTime -> startMillis.

fix_duration = True
# The field duration (in ms) was always 30s too long since it included the timeout (fixed now). -> Parse json and subtract 30000.

# Not fixed yet:
# - missing start messages first day (fixed 2019-03-13)


debug = False # will use the following test cases instead of stdin:
tests = [
  "Mar 12 10:05:16 sensors/shower/status { time: 6571, event: 'connect', clientId: 'ESP8266Client-5b08' }",
  'Mar 12 10:05:26 sensors/shower/flow { time: 16583, flow: 89, temp: 29.889368 }',
  'Mar 12 10:05:27 sensors/shower/flow { time: 17584, flow: 98, temp: 30.530787 }',
  'Mar 12 10:05:28 sensors/shower/flow { time: 18585, flow: 26, temp: 30.347132 }',
  'Mar 12 10:05:58 sensors/shower/stop { time: 48615, startTime: 0, duration: 48615, total_ml: 213 }',
  'Jan 06 20:21:46 sensors/shower/start { "millis": 3585903061 }',
  'Jan 06 20:21:46 sensors/shower/flow { "millis": 3585904062, "flow": 1, "temp": 21.116236 }',
  'Jan 06 20:22:16 sensors/shower/stop { "millis": 3585934092, "startMillis": 3585903061, "duration": 31031, "total_ml": 1 }',
  ]

lines = tests if debug else sys.stdin # can be iterated over

for line in lines:
  if not debug: line = line.rstrip('\n') # strip stdin line ending
  [prefix, msg] = line.split(' {', 1)
  msg = '{' + msg
  [ts, topic] = prefix.rsplit(' ', 1)
  if fix_date:
    year = '2020' if ts[:3] == 'Jan' else '2019'
    # date and time is with leading zeros, i.e. always the same length so we can just index into the string
    ts = str(datetime.strptime(ts[0:7]+year, '%b %d %Y').date()) + ts[6:]
  if fix_json:
    # ' -> ": change single to double quotes
    msg = msg.replace("'", '"')
    # add double quotes around unquoted keys
    msg = re.sub(r'([,{]) ([^"]+?):', r'\1 "\2":', msg)
    # rename fields
    msg = msg.replace('time', 'millis').replace('startTime', 'startMillis')
  if fix_duration:
    if 'duration' in msg: # check string instead of parsed object since it's costly
      o = json.loads(msg, object_pairs_hook=OrderedDict) # by default (w/o the hook) it does not keep the order of keys!
      assert(o['duration'] > 31000)
      o['duration'] -= 30000
      msg = json.dumps(o)
  print(ts, topic, msg)
