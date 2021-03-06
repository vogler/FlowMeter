# FlowMeter
Flow meter for tracking shower usage (every second: flow, temperature, after 30s of no flow: total volume, duration).

WiFi -> MQTT -> Telegraf -> InfluxDB -> Chronograf:
![chronograf](images/graph-chronograf.png)

First used a Wemos D1 mini, then a Doit ESP32, then a Wemos D1 mini again.

![flow meter](https://lh3.googleusercontent.com/vEOiO-a8hrksRDhh_PwxtEXgTaaAdvnz0TJxWVymXEPRcsADet_Zi1IfBUj9HUi4G0htsMyPE4IjZRi_4Fmnn2koxT4hHEFYEyFx1Y5yvNERDYzAIRjrDQNd6Sx2wiVZiJ9dPH6liVwH1zTr2wr5qbeAJaG0Q_-3Jbp7JHsNxb2tVhMpvvOTPBYSrObF3OR-2xMvVKD9FUeDjCaYlwPCjf_R4kAJf_2LSEYA2r2zdDgFYPHXFtvehkeWaN97Ulb1FIeE2o8ZpLJsHohVhYQg_5v-fSu3rmXj4rSJA5x34cD9LBPqAj8mLFTLbO815znV73eyjJPFT7yIsxSnoZ4ZK0QvkAobMnpn_x5W1Vag9AhDr3N_8Q25JPD5xYRPWzls-tmtFMJm79tfuqrACNiqJ6vF912FYQ1-yA27yNQABRVQWJIMswvqzqO28w7Aubx1-EFpT9aeidyfWGLYhq2HvbLdKLLBIs1KS0pfUilXWzv-eZlblyuqoRF86Ljpkl8gEGyu9QpKm9sGQ27dPWk1xjW4ECX_O_jtDburD-ffs86UvDNvwGv9FSqhy0CrEb6XDNuEznpc-TVzzxYwHxzp21D5v13ykk2lFhfQrWiYATP0GYVsHR7gZJSgeloaORy9ysE31DvsSyvBmPogA_rB_wb3wOHwa-7naRV-hyz8JjaEhxz15MUtSznDgE_9rAfBO-qkOtZMB8uCLj15-BQ3AQ2SWgio8ydiXMb74uNsZfqvWfUH=w2713-h2035-no)
![wemos d1 mini](https://lh3.googleusercontent.com/4L-H3jA39ISEGsOUjt5fDa_D4j3mfAaXzODmQWNnCm63av9RCV0-5JkDOxum7ILlEBoSZo00JnY6r0IJUMkvd93ET5m47DoZ3u-o9DjNFrzkzi_dqikcNdmUbfzglL9-GnLbaSYHLYcav2nO5-FvOoyMUBwUhcsUwxzlpaQitDEzNo_O_P8FT-8RiXJngoCKbvTygnDyfvGbbX5m7YZw6W9bAyjuTQ5k54OWivOFf3GpsE6zVAAME4HQpF2aO5HlOnbADQMbnDmcQdvXOs1Ep36s2MFWQSZ7XwvMnk_GCTx7HvP13hOnYUo_lkzyj5MIt-sX59IV8U2pmkE06vpmZf8uHaqZW0DFW4NCWBj2DLOAsOPQb6HokJgdUiPEXOpPHqeOEy878PEiXiGbb6mehpmSGcOV_k2eekgA6NprXK4N9neTrmiJyEo854vtvHhqds3m0vZqkjEfbmC4KXh60ObIFnMUpWw75SvYK0b7mowEM1vmNKIEPWaLgBA5wYG_97rtbmvTSPfo8Hw5tUbJFCbGn6yZZiM9rI4vUrn-iDi-696XG0X98SsV6fVIiwVQ0FPVzI4VoA7zrS2mho3t8LrJnTzbKMTte27hp_Vyv3ZIvSkwuzoLC99me7A50T47UYB23AWVtK4w3mhv2ODptGfbLY4U7xgOKEN9zoxLYn--iJDQp7gDHuOQhW89VD0TUZd8-yk8U3wmZaAgOaSt8vCsT3ICqYUusLFz08VLo97onRQs=w1527-h2035-no)
![doit esp32](https://lh3.googleusercontent.com/HzdVfPIto4MNtFWHyZxVwBgPlXlFsDTGzGDrqnveyxOkWbAWApKmcTECR-haP3UxJvIzTSdtNIhiNDgZnSUvPRkA3xfJHiDzgr3KtqJ2C88eTkAxM_o_b7gTB5raDm0mhDifz8-t_Yi8XFxR3iWstXEw4Mfz-s9VTR-i6wzYqi5eLRs6H1MLDNjvgWF6qzYHdLdKSeDJVblqgZb9H_1LY5l9cy1cnsxYZtp5UcgpNJDdHO1QLQhOPpbU_Ptd4VmZSXs2oAz8kgZczZmGj1YfIrbn3wGioNjaxKl1X4BF9ZXvWl-qq77RsPyONQli4nfhFExJgwWPNYdo4E9VvR9rKZpI385hwgm52STGDmiPkt-tbMiA0AKLaha1D9eTxGNEoPZKJPLC6Thh6vlA7MDvQgLfaxidZxknSrm_6UFkekVa61c-_w-J4eHiKC9LZ3sb5L5h1tltaf2mQ-lqhFXXwetNumzbl5JmGYMPGHG_h3RkvHXRRTDK7e6aulF2AfDEx5TO7H4I9vX9pUJXWmxfb0-CJS5tEHDpGB94kLB64NTOVZpty5Uen9GcImEMOPKO79fLkaYHTwrXwVAwI27J4lPp6vynbSwxSTDKWXOCuYBFOf0VFen4Pb7ODOSx5iDdbJwPIwgIcYDylVI5zRaFBhVs1T61bWvmhnbLE1ZeF5PDfdPUDT9iquZi7es3lEmxcJFxp9lmPbdRtL5VbWzbtLxoPucd1_0jLFD6ci5OVhbWUmbe=w1527-h2035-no)

![sketch](images/Sketch_bb.png)

All photos/videos: https://photos.app.goo.gl/8Zbm7jH6h51TRUgj6

See [log.md](log.md) for history/development notes.
