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

| Machine            | Bytes 3-6     | Source                               |
| ------------------ | ------------- | ------------------------------------ |
| EP2200 / EP2220/10 | `00 00 03 02` | upstream issues #10, #90             |
| unknown            | `01 00 00 02` | upstream issue #19                   |
| EP2220 / EP2235    | `01 02 00 02` | upstream `protocol.md`               |
| EP2231             | `01 02 00 03` | captured from the display, this fork |
| EP2230             | `01 03 00 0D` | upstream issue #74                   |
| EP3243 / EP3246    | `01 03 00 0E` | upstream `commands.h`                |
| EP3241             | `01 03 00 12` | upstream issues #44, #75             |
| EP3221             | `01 04 02 04` | upstream issues #36, #40             |

Between the EP2220 and the EP2231 only byte 6 differs, but the EP2200 and the EP3221 show that
the whole block varies, so treat it as one opaque machine identifier rather than a single
variant byte. All messages behind this table are collected in [captures.csv](captures.csv).

### Checksum

The last 2 bytes are a 12-bit checksum, two 6-bit values. Throughout this section a checksum is
written as a single number, `(byte[n-2] << 6) | byte[n-1]`.

The checksum is **linear over GF(2)**: it is the XOR of a constant and one fixed 12-bit weight
per byte value present in the message. Flipping a bit anywhere always changes the checksum by
the same amount, independent of everything else.

The weight of a byte depends on its **distance from the end of the message**, not on its offset
from the start, and **both directions share one weight table**. That is what makes the model
usable: the 19 byte mainboard frames and the 12 byte display commands constrain each other.
The alignment was found by fitting both directions separately and comparing:

| Distance from end | Byte value | From the display commands | From the mainboard frames |
| ----------------- | ---------- | ------------------------- | ------------------------- |
| 1                 | `07`       | `966`                     | `966`                     |
| 1                 | `38`       | `3E3`                     | `3E3`                     |
| 2                 | `38`       | `05B`                     | `05B`                     |

#### Weight table

Distance `d` counts back from the last content byte (the byte before the checksum), so `d = 0`
is byte 9 of a display command and byte 16 of a mainboard frame. Blank means that value has
never been observed at that distance, and its weight is unknown.

| d   | `01`  | `02`  | `03`  | `04`  | `07`  | `08`  | `0E`  | `10`  | `12`  | `20`  | `38`  | `3F`  |
| --- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
| 0   | `204` |       |       |       | `E5C` |       |       |       |       |       |       |       |
| 1   | `30C` | `619` | `515` | `C73` | `966` | `AA2` | `0C8` | `700` | `119` | `E41` | `3E3` | `A85` |
| 2   | `30D` | `61B` | `516` | `C37` | `921` | `A2A` | `006` | `610` | `00B` | `C61` | `05B` | `97A` |
| 3   | `B5D` | `6BB` | `DE6` |       |       |       |       |       |       |       |       |       |
| 4   |       |       | `3B7` | `EEE` | `D59` |       |       |       |       |       |       |       |
| 5   |       | `422` |       |       | `432` |       |       |       |       |       | `CC8` | `8FA` |
| 6   | `8A2` |       |       |       | `446` |       |       |       |       |       | `232` | `674` |
| 7   | `D11` | `A63` | `772` | `6C3` | `1B1` | `D87` | `127` |       |       |       | `056` | `1E7` |
| 8   |       |       |       |       | `AAC` |       |       |       |       |       | `FF7` | `55B` |
| 9   |       |       |       |       |       |       |       |       |       |       |       |       |
| 10  |       |       | `F9E` | `454` | `BCA` |       |       |       |       |       |       |       |
| 11  |       |       | `9C1` | `D43` | `482` |       |       |       |       |       |       |       |
| 12  |       |       | `A73` | `AC1` | `0B2` |       |       |       |       |       |       |       |
| 13  |       |       | `A23` | `A7F` | `05C` |       |       |       |       |       |       |       |
| 14  | `FEB` |       |       |       |       |       |       |       |       |       |       |       |

Weights are additive in the byte value where the parts are known, e.g. `W(d, 0x3F)` equals
`W(d, 0x07) XOR W(d, 0x38)`. The gaps are not oversights, they are values that never occur:
`d = 9` is byte 7 of a mainboard frame, which stayed `00` across every captured state.

#### Computing a checksum

```
checksum = K XOR (weight of every content byte at its distance from the end)
```

For **mainboard to display** frames `K = E4D`, which is simply the checksum of the all-zero
frame. Those frames can be computed outright.

For **display to mainboard** commands `K` is not observable. It would be the checksum of a
message whose machine identifier is `00 00 00 00`, and no such machine exists, so `K` and the
weights of bytes 3-6 only ever appear added together. One captured message from the machine
pins that sum, and from there its whole command set follows:

```
checksum(new) = checksum(anchor) XOR W(d, new[d]) XOR W(d, anchor[d])   for every byte that differs
```

A status request (`D5 55 00 <machine> 00 00 00 <checksum>`) is the easiest anchor, since the
display sends it continuously while the machine is on.

#### How well this is established

The model was fitted over 143 messages: 81 distinct mainboard frames and 10 display commands
captured on an EP2231 for this fork, plus every checksummed message that could be harvested
from the upstream repository and its issue tracker, covering 8 machines. The system has 44
degrees of freedom, so **99 of those messages are redundant** — they had to come out right and
did.

Exactly two messages contradict everything else:

```
D5 55 0A 01 04 02 04 00 00 00 2A 10   upstream issue #36
D5 55 01 01 04 02 04 00 00 00 01 25   upstream issue #36
```

Their checksums are byte-identical to the `01 03 00 0E` values for the same instruction, so
these are almost certainly machine bytes and a checksum taken from different messages. They are
kept in [captures.csv](captures.csv) with a note rather than silently dropped.

Two blind tests:

- Fitted on one capture session, then asked to predict a later one: 7 of 9 new frames predicted
  exactly. The other 2 were the first frames ever seen with the pre-ground-coffee LED, a bit
  combination outside the fitted span — the model reported them as not computable instead of
  guessing wrong.
- Each machine removed from the training set entirely and handed back a **single** captured
  message as anchor, then asked for the rest of its command set: **52 of 52 correct, none
  wrong**, across 6 machines.

#### It is not a CRC

Worth recording so nobody repeats the search. If the checksum were any 12-bit CRC or LFSR, the
contribution of message bit `b` at byte position `p` would be `x^(offset + w*p + b) mod g`, up
to a fixed invertible output map — and that map absorbs bit order, byte packing, reflection,
init value and final XOR. So the test is: compute the raw polynomial value per message, then ask
whether one single matrix maps all of them onto the measured checksums.

Searched exhaustively: all 4096 polynomials, 6 and 8 bits per symbol, both bit orders, both byte
orders. **No candidate survives.** The same search run against synthetic data from a real CRC-12
recovers its polynomial uniquely, so this is a real negative and not a broken search.

The weaker model "12-bit register, `reg = T*reg XOR V(byte)` per byte" fails too: no single `T`
is compatible with the weight table.

What remains is a hard-wired weight matrix, and the table above is all there is to know about it.

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

Possible: set both button bits and compute the checksum with the model described above. Both
bits sit at distances 1 and 2, where every single-bit weight is known, so the checksum follows
without a new capture. Untested on hardware.

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
| 7    |                   | unknown, never non-zero                               |
| 8    | Beans-LED         | `00` - 1 LED; `38` - 2 LEDs; `3F` - 3 LEDs            |
| 9    | Beans-LED         | `07` - show led group; `38` - powder selected         |
| 10   | Size-LED          | `00` - 1 LED; `38` - 2 LEDs; `3F` - 3 LEDs            |
| 11   | Size-LED          | `07` - show led group; `38` - aqua clean selected     |
| 12   |                   | `07` seen with all four beverage LEDs lit             |
| 13   |                   | unknown, never non-zero                               |
| 14   | Water Empty       | `38` - water                                          |
| 15   | Waste&Warning-LED | `07` - waste; `38` - warning sign                     |
| 16   | Play/Pause-LED    | `07` - on                                             |
| 17   | checksum          |
| 18   | checksum          |

Aqua clean does not use bytes 12 or 13 as previously assumed. Selecting it lights byte 11 bits
3-5 while every beverage LED is dark:

```
D5 55 00 00 00 00 00 00 00 00 00 38 00 00 00 00 00 0A 05
```

Bytes 7 and 13 stayed `00` across ~80 distinct frames covering power up and down, every drink,
pre-ground coffee, brewing, the water/waste/warning indicators and aqua clean.

## Off LED states

`D5 55 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 39 0D`

## Idle LED states

`D5 55 00 07 07 07 07 00 00 00 00 00 00 00 00 00 00 07 2B`
