# Communication protocol

The underlining protocol used by this coffee machine is quite simple.
The mainboard does all the heavy lifting and the display unit is only responsible for I/O.
Upon powering on, the Display unit issues a 'power on' command and then continues to poll the machine status from the mainboard using a request message.
The mainboard replies to this request with a message containing all LED states.
If the user presses a button this this information is passed along in a message to the mainboard. Once the user releases the button the display returns to sending status requests.
If the machine is off/sleeping no messages are sent in either direction.
The messages were obtained by listening to the bus.

## Messages from the display to the mainboard

All messages are 12 bytes long. The length is not encoded but it also never changes.

| Byte  | Purpose      | Detail                                                                                                                      |
| ----- | ------------ | --------------------------------------------------------------------------------------------------------------------------- |
| 0     | START        | always `D5`                                                                                                                 |
| 1     | START        | always `55`                                                                                                                 |
| 2     | INSTRUCTION  | `00` - idle/button press; `01` - power on without cleaning; `02` - power on with cleaning; `0A` - pre power on (beeps only) |
| 3-6   | MACHINE      | identifies the machine, see below                                                                                           |
| 7     | Drink button | `02` - espresso; `04` - hot water; `08` - coffee; `10` - steam/cappuccino                                                   |
| 8     | Settings     | `02` - bean; `04` - size; `10` - aqua clean; `20` - calc clean                                                              |
| 9     | Play/Pause   | `01` - pressed                                                                                                              |
| 10-11 | checksum     |                                                                                                                             |

Byte 7 also carries `01` for the power button, which is how the power off message is encoded.

### Machine identification

Bytes 3-6 are constant for a given machine but differ between models. The mainboard latches
them at power on and configures itself accordingly, so injecting the wrong ones leaves it
configured for a different machine: on an EP2231 sent the EP2220 value, coffee and espresso
still worked but hot water and cappuccino did not, for the rest of the session.

| Machine         | Bytes 3-6     | Source                                                                                      |
| --------------- | ------------- | ------------------------------------------------------------------------------------------- |
| EP2220 / EP2235 | `01 02 00 02` | upstream                                                                                    |
| EP2231          | `01 02 00 03` | captured from the display                                                                   |
| EP3243 / EP3246 | `01 03 00 0E` | upstream                                                                                    |
| EP2220/10       | `00 00 03 02` | [upstream issue #90](https://github.com/TillFleisch/ESPHome-Philips-Smart-Coffee/issues/90) |

Between the EP2220 and the EP2231 only byte 6 differs, but the EP2220/10 shows that the whole
block varies, so treat it as one opaque machine identifier rather than a single variant byte.

### Checksum

The last 2 bytes are a 12-bit checksum, two 6-bit values. The algorithm is not known and does
not appear to be a standard CRC.

It is however **linear**: flipping a bit anywhere in the message always changes the checksum by
the same fixed amount, independent of the rest of the message. Given one captured message, the
checksum of another message is therefore

```
checksum(new) = checksum(captured) XOR (contributions of the changed bits)
```

The contributions can be read straight off the tables below by XOR-ing a command against the
status request of the same machine. This is what the EP2231 table below was generated with; it
reproduced all 21 checksums that were later measured, 12 of them from the EP3243 family which
took no part in deriving the contributions.

This is not the algorithm. It needs one measured message from a machine as an anchor, and it
only covers bits that vary somewhere in the samples — bytes 3-6 are effectively unreachable
this way, which is why a new machine has to be captured rather than computed. It also says
nothing about the 19 byte mainboard-to-display frames.

The messages listed below were captured on an EP2220.

### Power on message

There seem to be 3 messages responsible for powering on the machine.

`D5 55 0A 01 02 00 02 00 00 00 0E 12`

Makes the machine beep, but does not actually power on the machine.

`D5 55 01 01 02 00 02 00 00 00 25 27`

Beeps and powers on the machine, but without a cleaning cycle during startup.

`D5 55 02 01 02 00 02 00 00 00 38 15`

Beeps, powers on the machine and includes a cycle during startup.

The last 2 Messages are used within this project.
The 3rd byte is likely responsible for turning the coffee machine on, since it's `00` in all other messages.

### Power off message

`D5 55 00 01 02 00 02 01 00 00 1D 3B`

When sent, the mainboard proceeds to shut down the machine. The display unit goes to sleep after powering off.

### Status request message

`D5 55 00 01 02 00 02 00 00 00 11 36`

### Play/Pause button message

`D5 55 00 01 02 00 02 00 00 01 19 32`

### Drink selection

Byte nr. 8 is used to transmit drink selections. The 4 least significant bits are used. The bit indices and the resulting checksums are listed below.
Button|Message|
-|-
Espresso| `D5 55 00 01 02 00 02 02 00 00 09 2D`
Hot Water|`D5 55 00 01 02 00 02 04 00 00 21 01`
coffee|`D5 55 00 01 02 00 02 08 00 00 39 1C`
cappuccino|`D5 55 00 01 02 00 02 10 00 00 09 26`

### Settings buttons

The 9th byte is used to transmit the right hand side button group in a similar fashion.

| Button     | Message                               |
| ---------- | ------------------------------------- |
| Beans      | `D5 55 00 01 02 00 02 00 02 00 09 2F` |
| Size       | `D5 55 00 01 02 00 02 00 04 00 20 05` |
| Aqua clean | `D5 55 00 01 02 00 02 00 10 00 0D 36` |
| Calc clean | `D5 55 00 01 02 00 02 00 20 00 28 37` |

### Play/Pause button

`D5 55 00 01 02 00 02 00 00 01 19 32`

### Encoding simultaneous button presses

Possible: set both button bits and compute the checksum with the linear model described above.
Untested on hardware.

## EP2231 command set

Captured from the display of an EP2231 (LatteGo). Same layout as the EP2220, bytes 3-6 are
`01 02 00 03`. `Steam` is replaced by `Cappuccino`.

| Message                   | Bytes                                 | Origin   |
| ------------------------- | ------------------------------------- | -------- |
| Status request            | `D5 55 00 01 02 00 03 00 00 00 3C 2B` | captured |
| Pre power on              | `D5 55 0A 01 02 00 03 00 00 00 23 0F` | captured |
| Power on without cleaning | `D5 55 01 01 02 00 03 00 00 00 08 3A` | captured |
| Power on with cleaning    | `D5 55 02 01 02 00 03 00 00 00 15 08` | derived  |
| Power off                 | `D5 55 00 01 02 00 03 01 00 00 30 26` | captured |
| Espresso                  | `D5 55 00 01 02 00 03 02 00 00 24 30` | captured |
| Hot water                 | `D5 55 00 01 02 00 03 04 00 00 0C 1C` | captured |
| Coffee                    | `D5 55 00 01 02 00 03 08 00 00 14 01` | captured |
| Cappuccino                | `D5 55 00 01 02 00 03 10 00 00 24 3B` | captured |
| Bean                      | `D5 55 00 01 02 00 03 00 02 00 24 32` | captured |
| Size                      | `D5 55 00 01 02 00 03 00 04 00 0D 18` | captured |
| Aqua clean                | `D5 55 00 01 02 00 03 00 10 00 20 2B` | derived  |
| Calc clean                | `D5 55 00 01 02 00 03 00 20 00 05 2A` | derived  |
| Play/Pause                | `D5 55 00 01 02 00 03 00 00 01 34 2F` | derived  |

The display picks the power on variant itself: it sends `01` when the machine was on recently
and `02` after it has been off for a while.

## Messages from the mainboard to the display

All messages have the following structure:

| Start Message | Content                                        | Checksum |
| ------------- | ---------------------------------------------- | -------- |
| `D5     55`   | `00 00 00 00 00 00 00 00 00 00 00 00 00 00 00` | `39 0D`  |

The structure is similar to the previous messages but the content part is longer. The message above means all LEDs on the display are turned off.
The following table show the purpose of each byte and their known states

| Byte | Purpose           | Detail                                                |
| ---- | ----------------- | ----------------------------------------------------- |
| 0    | START             |
| 1    | START             |
| 2    | INSTRUCTION       |
| 3    | Espresso-LED      | `03`/`07` - half/full brightness ; `38` - 2x espresso |
| 4    | Hot Water-LED     | `03`/`07` - half/full brightness                      |
| 5    | Coffee-LED        | `03`/`07` - half/full brightness; `38` - 2x coffee    |
| 6    | Cappuccino-LED    | `03`/`07` - half/full brightness                      |
| 7    |                   | unknown                                               |
| 8    | Beans-LED         | `00` - 1 LED; `38` - 2 LEDs; `3F` - 3 LEDs            |
| 9    | Beans-LED         | `07` - show led group; `38` - powder selected         |
| 10   | Size-LED          | `00` - 1 LED; `38` - 2 LEDs; `3F` - 3 LEDs            |
| 11   | Size-LED          | `07` - show led group                                 |
| 12   |                   | probably aqua_clean/calc_clean                        |
| 13   |                   | probably aqua_clean/calc_clean                        |
| 14   | Water Empty       | `38` - water                                          |
| 15   | Waste&Warning-LED | `07` - waste; `38` - warning sign                     |
| 16   | Play/Pause-LED    | `07` - on                                             |
| 17   | checksum          |
| 18   | checksum          |

## Off LED states

`D5 55 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 39 0D`

## Idle LED states

`D5 55 00 07 07 07 07 00 00 00 00 00 00 00 00 00 00 07 2B`
