- 12.11.2018: [ordered](https://trade.aliexpress.com/order_detail.htm?spm=a2g0s.9042311.0.0.27424c4dmRTqEt&orderId=96461396622588) 1st flow sensor (male-female, 6.55cm) for 7.10€
  - [1/2" YF-B4 NTC Brass water Hall flow sensor flow rate temperature measurement METER](https://de.aliexpress.com/item/32787385391.html?spm=a2g0s.9042311.0.0.27424c4dTLcSpa)
  - arrived on ??
  - Only noticed then that it's not bidirectional and this one is exactly the wrong way around, such that I need to use two adapters for it.
- 23.01.2019: [ordered](https://trade.aliexpress.com/order_detail.htm?spm=a2g0s.9042311.0.0.27424c4dmRTqEt&orderId=98372476502588) 2nd flow sensor (male-male, 4.4cm) for 8.48€
  - [water flow sensor meter indicator USC-HS21TIT brass hall turbine flowmeter temperature sensor 1-30L/min male G1/2" DN15 44mm](https://de.aliexpress.com/item/32576247103.html?spm=a2g0s.9042311.0.0.27424c4dTLcSpa)
  - arrived on ??
- 17.01.2020: Wemos D1 mini stopped working because it rusted because water was dripping from the ceiling (not from normal showering but leak from apartment above). See [rust-1.jpg](images/rust-1.jpg), [rust-2.jpg](images/rust-2.jpg).
- 18.01.2020: installed replacement Wemos D1 mini (now on small breadboard with most wiring done under the PCB).
  - Fixed bug: duration was 30s too long (included timeout).
  - Fixed reporting of flow when light switch is turned on/off by ignoring the first pulse interval. TODO: might need to ignore up to 3 intervals ({1: 3156, 2: 147, 3: 3, 4: 0}).
  - Flow worked, but temperatures are way too high > 110-147 C.
- 20.01.2020: debugged temperature issue.
  - A0 is not working right: ~2.8 V measured between GND and A0, when shorted it should report 0, but it led to reboot. 3V3-A0 works and reports 1023. Seems like A0 is connected to something it shouldn't be. Header pins are soldered fine.
  - Soldered header pins to another replacement Wemos D1 mini. Everything works as expected: A0-3V3 reports 1023, A0-GND reports 0.
  - Temperatures seem as before. Took the chance to compare:
    Directly measured NTC thermistor's resistance, temperature from table (cut-off):
      middle:  34 kOhm -> ?
      coldest: 84.60 kOhm -> 13.5 C
      warmest: 13.57 kOhm -> 54 C?
    Temperatures IR:NTC (IR temp. gun aimed at bathtub with shower head ~5cm above : calculated temp.)
      middle:  33.6:36.6 33.1:36.2
      coldest: 11.0:12.60 10.6:12.24 10.3:11.97
      warmest: 52.3:61.29 52.3:62.16
    Values seem good enough. Maybe water cools off between sensor and shower head.
  - Switched to 2nd flow sensor since it's shorter (see [flow-sensors.jpg](images/flow-sensors.jpg) for comparison) and have not tried it yet.
    - Flow seems to be the same. Resistance of thermistor is slightly different: ~32kOhm@34C, 77kOhm@10C, 20kOhm@55C.
    - Resistance is much less responsive than for the other flow sensor. This one seems to have NTC on the outside of the metal, where the other one has it on a pin that is screwed in so that it's in the stream.
    - E.g. changed water from 10C to 54C (~5s IR temp.), resistance took >1min to change from 77kOhm to 28kOhm, would have taken even longer to reach 20kOhm. The other flow sensor (84.6-13.6kOhm range) went from 80kOhm to 24kOhm in 10s and to 16kOhm in 20s.
    - -> changed back to 1st flow sensor
- 22.01.2020:
  - fix-logs.py to normalize logs
  - fix-logs.sh to delete spurious start/flow{1,3}/stop sequences, kept ~/shower.org.log (after fix-logs.py) to drop diff from InfluxDB (TODO)
