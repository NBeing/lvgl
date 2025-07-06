# ASM Hydrasynth - MIDI CC Parameter Reference

## Overview
**Manufacturer:** Ashun Sound Machines  
**Model:** Hydrasynth Desktop/Keyboard  
**Firmware Version:** 1.5.5  
**Type:** 8-voice polyphonic wavetable synthesizer with mutators  

## Parameter Categories

### System Controls

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Bank Select MSB | Bank MSB | 0 | 0 to 127 | 0 | Bank select most significant byte |
| Modulation Wheel | Mod Wheel | 1 | 0 to 127 | 0 | Standard modulation wheel control |
| Master Volume | Volume | 7 | 0 to 127 | 100 | Global volume control |
| Expression Pedal | Expression | 11 | 0 to 127 | 127 | Expression pedal control |
| Bank Select LSB | Bank LSB | 32 | 0 to 127 | 0 | Bank select least significant byte |
| Sustain Pedal | Sustain | 64 | 0 to 127 | 0 | Sustain pedal on/off |
| All Notes Off | Notes Off | 123 | 0 to 127 | 0 | Emergency all notes off |

### Voice Controls

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Glide Time | Glide Time | 5 | 0 to 127 | 0 | Portamento/glide time between notes |
| Glide | Glide | 66 | 0 to 127 | 0 | Glide on/off and mode |
| Detune | Detune | 95 | 0 to 127 | 64 | Global detuning of voices |
| Stereo Width | Stereo | 117 | 0 to 127 | 64 | Stereo field width control |

### Oscillators

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| OSC 1 Volume | O1 Vol | 44 | 0 to 127 | 100 | Volume level of oscillator 1 |
| OSC 1 Pan | O1 Pan | 45 | 0 to 127 | 64 | Stereo panning of oscillator 1 |
| OSC 1 Wavscan | O1 Wave | 24 | 0 to 127 | 0 | Wavetable scanning position for oscillator 1 |
| OSC 1 Cent | O1 Cent | 111 | 0 to 127 | 64 | Fine tuning in cents for oscillator 1 |
| OSC 1 FRate | O1 FRate | 118 | 0 to 127 | 64 | Frequency rate modulation for oscillator 1 |
| OSC 2 Volume | O2 Vol | 46 | 0 to 127 | 100 | Volume level of oscillator 2 |
| OSC 2 Pan | O2 Pan | 47 | 0 to 127 | 64 | Stereo panning of oscillator 2 |
| OSC 2 Wavscan | O2 Wave | 26 | 0 to 127 | 0 | Wavetable scanning position for oscillator 2 |
| OSC 2 Cent | O2 Cent | 112 | 0 to 127 | 64 | Fine tuning in cents for oscillator 2 |
| OSC 2 FRate | O2 FRate | 119 | 0 to 127 | 64 | Frequency rate modulation for oscillator 2 |
| OSC 3 Volume | O3 Vol | 48 | 0 to 127 | 100 | Volume level of oscillator 3 |
| OSC 3 Pan | O3 Pan | 49 | 0 to 127 | 64 | Stereo panning of oscillator 3 |
| OSC 3 Cent | O3 Cent | 113 | 0 to 127 | 64 | Fine tuning in cents for oscillator 3 |
| OSC 3 FRate | O3 FRate | 114 | 0 to 127 | 64 | Frequency rate modulation for oscillator 3 |

### Mixer

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Noise Volume | Noise Vol | 3 | 0 to 127 | 0 | Volume level of noise generator |
| Noise Pan | Noise Pan | 8 | 0 to 127 | 64 | Stereo panning of noise generator |
| Noise FRate | Noise FRate | 115 | 0 to 127 | 64 | Frequency rate modulation for noise |
| Ring Mod Volume | RM Vol | 9 | 0 to 127 | 0 | Volume level of ring modulator |
| Ring Mod Pan | RM Pan | 10 | 0 to 127 | 64 | Stereo panning of ring modulator |
| RM12 Depth | RM12 Depth | 43 | 0 to 127 | 0 | Ring modulation depth between OSC1 and OSC2 |
| RM12 FRate | RM12 FRate | 116 | 0 to 127 | 64 | Frequency rate modulation for ring modulator |

### Filters

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Filter 1 Cutoff | F1 Cutoff | 74 | 0 to 127 | 64 | Cutoff frequency of filter 1 |
| Filter 1 Resonance | F1 Res | 71 | 0 to 127 | 0 | Resonance amount of filter 1 |
| Filter 1 Drive | F1 Drive | 50 | 0 to 127 | 0 | Drive/saturation amount for filter 1 |
| Filter 1 Keytrack | F1 Key | 51 | 0 to 127 | 64 | Keyboard tracking for filter 1 |
| Filter 1 LFO1 Amount | F1 LFO1 | 52 | 0 to 127 | 0 | LFO1 modulation amount for filter 1 |
| Filter 1 Vel Env | F1 Vel | 53 | 0 to 127 | 64 | Velocity envelope amount for filter 1 |
| Filter 1 ENV1 Amount | F1 ENV1 | 54 | 0 to 127 | 64 | Envelope 1 modulation amount for filter 1 |
| Filter 2 Cutoff | F2 Cutoff | 55 | 0 to 127 | 64 | Cutoff frequency of filter 2 |
| Filter 2 Resonance | F2 Res | 56 | 0 to 127 | 0 | Resonance amount of filter 2 |
| Filter 2 Type | F2 Type | 57 | 0 to 127 | 0 | Filter type selection for filter 2 |
| Filter 2 Keytrack | F2 Key | 58 | 0 to 127 | 64 | Keyboard tracking for filter 2 |
| Filter 2 LFO1 Amount | F2 LFO1 | 59 | 0 to 127 | 0 | LFO1 modulation amount for filter 2 |
| Filter 2 Vel Env | F2 Vel | 60 | 0 to 127 | 64 | Velocity envelope amount for filter 2 |
### Envelopes

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| ENV 1 Attack | E1 Att | 81 | 0 to 127 | 0 | Attack time of envelope 1 |
| ENV 1 Decay | E1 Dec | 82 | 0 to 127 | 64 | Decay time of envelope 1 |
| ENV 1 Sustain | E1 Sus | 83 | 0 to 127 | 100 | Sustain level of envelope 1 |
| ENV 1 Release | E1 Rel | 84 | 0 to 127 | 32 | Release time of envelope 1 |
| ENV 2 Attack | E2 Att | 85 | 0 to 127 | 0 | Attack time of envelope 2 |
| ENV 2 Decay | E2 Dec | 86 | 0 to 127 | 64 | Decay time of envelope 2 |
| ENV 2 Sustain | E2 Sus | 87 | 0 to 127 | 100 | Sustain level of envelope 2 |
| ENV 2 Release | E2 Rel | 88 | 0 to 127 | 32 | Release time of envelope 2 |
| ENV 3 Attack | E3 Att | 89 | 0 to 127 | 0 | Attack time of envelope 3 |
| ENV 3 Decay | E3 Dec | 90 | 0 to 127 | 64 | Decay time of envelope 3 |
| ENV 3 Sustain | E3 Sus | 96 | 0 to 127 | 100 | Sustain level of envelope 3 |
| ENV 3 Release | E3 Rel | 97 | 0 to 127 | 32 | Release time of envelope 3 |
| ENV 4 Attack | E4 Att | 25 | 0 to 127 | 0 | Attack time of envelope 4 |
| ENV 4 Decay | E4 Dec | 27 | 0 to 127 | 64 | Decay time of envelope 4 |
| ENV 4 Sustain | E4 Sus | 125 | 0 to 127 | 100 | Sustain level of envelope 4 |
| ENV 4 Release | E4 Rel | 124 | 0 to 127 | 32 | Release time of envelope 4 |
| ENV 5 Attack | E5 Att | 102 | 0 to 127 | 0 | Attack time of envelope 5 |
| ENV 5 Decay | E5 Dec | 103 | 0 to 127 | 64 | Decay time of envelope 5 |
| ENV 5 Sustain | E5 Sus | 104 | 0 to 127 | 100 | Sustain level of envelope 5 |
| ENV 5 Release | E5 Rel | 105 | 0 to 127 | 32 | Release time of envelope 5 |

### LFOs

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| LFO 1 Rate | LFO1 Rate | 72 | 0 to 127 | 64 | Rate/speed of LFO 1 |
| LFO 1 Gain | LFO1 Gain | 70 | 0 to 127 | 0 | Output level of LFO 1 |
| LFO 2 Rate | LFO2 Rate | 73 | 0 to 127 | 64 | Rate/speed of LFO 2 |
| LFO 2 Gain | LFO2 Gain | 28 | 0 to 127 | 0 | Output level of LFO 2 |
| LFO 3 Rate | LFO3 Rate | 76 | 0 to 127 | 64 | Rate/speed of LFO 3 |
| LFO 3 Gain | LFO3 Gain | 75 | 0 to 127 | 0 | Output level of LFO 3 |
| LFO 4 Rate | LFO4 Rate | 78 | 0 to 127 | 64 | Rate/speed of LFO 4 |
| LFO 4 Gain | LFO4 Gain | 77 | 0 to 127 | 0 | Output level of LFO 4 |
| LFO 5 Rate | LFO5 Rate | 80 | 0 to 127 | 64 | Rate/speed of LFO 5 |
| LFO 5 Gain | LFO5 Gain | 79 | 0 to 127 | 0 | Output level of LFO 5 |

### Mutators

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Mutator 1 Ratio | M1 Ratio | 29 | 0 to 127 | 64 | Frequency ratio for mutator 1 |
| Mutator 1 Depth | M1 Depth | 30 | 0 to 127 | 0 | Processing depth for mutator 1 |
| Mutator 1 Dry/Wet | M1 Mix | 31 | 0 to 127 | 64 | Dry/wet mix for mutator 1 |
| Mutator 2 Ratio | M2 Ratio | 33 | 0 to 127 | 64 | Frequency ratio for mutator 2 |
| Mutator 2 Depth | M2 Depth | 34 | 0 to 127 | 0 | Processing depth for mutator 2 |
| Mutator 2 Dry/Wet | M2 Mix | 35 | 0 to 127 | 64 | Dry/wet mix for mutator 2 |
| Mutator 3 Ratio | M3 Ratio | 36 | 0 to 127 | 64 | Frequency ratio for mutator 3 |
| Mutator 3 Depth | M3 Depth | 37 | 0 to 127 | 0 | Processing depth for mutator 3 |
| Mutator 3 Dry/Wet | M3 Mix | 39 | 0 to 127 | 64 | Dry/wet mix for mutator 3 |
| Mutator 4 Ratio | M4 Ratio | 40 | 0 to 127 | 64 | Frequency ratio for mutator 4 |
| Mutator 4 Depth | M4 Depth | 41 | 0 to 127 | 0 | Processing depth for mutator 4 |
| Mutator 4 Dry/Wet | M4 Mix | 42 | 0 to 127 | 64 | Dry/wet mix for mutator 4 |

### Macros

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Macro 1 | Macro 1 | 16 | 0 to 127 | 64 | Assignable macro control 1 |
| Macro 2 | Macro 2 | 17 | 0 to 127 | 64 | Assignable macro control 2 |
| Macro 3 | Macro 3 | 18 | 0 to 127 | 64 | Assignable macro control 3 |
| Macro 4 | Macro 4 | 19 | 0 to 127 | 64 | Assignable macro control 4 |
| Macro 5 | Macro 5 | 20 | 0 to 127 | 64 | Assignable macro control 5 |
| Macro 6 | Macro 6 | 21 | 0 to 127 | 64 | Assignable macro control 6 |
| Macro 7 | Macro 7 | 22 | 0 to 127 | 64 | Assignable macro control 7 |
| Macro 8 | Macro 8 | 23 | 0 to 127 | 64 | Assignable macro control 8 |

### Arpeggiator

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| ARP Division | ARP Div | 106 | 0 to 127 | 64 | Note division/timing for arpeggiator |
| ARP Gate | ARP Gate | 107 | 0 to 127 | 100 | Gate length for arpeggiator notes |
| ARP Mode | ARP Mode | 108 | 0 to 127 | 0 | Arpeggiator pattern mode |
| ARP Ratchet | ARP Ratch | 109 | 0 to 127 | 0 | Ratcheting/repeat probability |
| ARP Chance | ARP Chance | 110 | 0 to 127 | 127 | Note trigger probability |
| ARP Octave | ARP Oct | 120 | 0 to 127 | 64 | Octave range for arpeggiator |
| ARP Length | ARP Len | 122 | 0 to 127 | 64 | Pattern length for arpeggiator |

### Effects

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Pre-FX Param 1 | PreFX P1 | 12 | 0 to 127 | 64 | First parameter for pre-filter effect |
| Pre-FX Param 2 | PreFX P2 | 13 | 0 to 127 | 64 | Second parameter for pre-filter effect |
| Pre-FX Mix | PreFX Mix | 93 | 0 to 127 | 0 | Dry/wet mix for pre-filter effect |
| Post-FX Param 1 | PostFX P1 | 68 | 0 to 127 | 64 | First parameter for post-filter effect |
| Post-FX Param 2 | PostFX P2 | 69 | 0 to 127 | 64 | Second parameter for post-filter effect |
| Post-FX Mix | PostFX Mix | 94 | 0 to 127 | 0 | Dry/wet mix for post-filter effect |
| Delay Feedback | Dly Feed | 14 | 0 to 127 | 32 | Feedback amount for delay effect |
| Delay Time | Dly Time | 15 | 0 to 127 | 64 | Delay time setting |
| Delay Wet Tone | Dly Tone | 63 | 0 to 127 | 64 | Tone control for delayed signal |
| Delay Dry/Wet | Dly Mix | 92 | 0 to 127 | 0 | Dry/wet mix for delay effect |
| Reverb Time | Rev Time | 65 | 0 to 127 | 64 | Reverb decay time |
| Reverb Tone | Rev Tone | 67 | 0 to 127 | 64 | Tone control for reverb |
| Reverb Dry/Wet | Rev Mix | 91 | 0 to 127 | 0 | Dry/wet mix for reverb effect |

### Amplitude

| Parameter | Short Name | CC# | Range | Default | Description |
|-----------|------------|-----|--------|---------|-------------|
| Amp LFO2 Amount | Amp LFO2 | 62 | 0 to 127 | 0 | LFO2 modulation amount for amplitude |

## Notes

- **Parameter Organization**: The Hydrasynth features a comprehensive modular architecture with 3 wavetable oscillators, dual filters, 5 envelopes, 5 LFOs, and 4 unique mutator processors.

- **Wavetable Scanning**: The Wavscan parameters (CC 24, 26, 126) control the position within the currently loaded wavetable. Each oscillator can independently scan through different wavetables.

- **Mutators**: The 4 mutator processors are unique wave shaping modules that can dramatically alter the character of the oscillators. Each has ratio, depth, and dry/wet controls.

- **Macro Controls**: The 8 macro controls (CC 16-23) are user-assignable and can be routed to multiple parameters simultaneously with custom ranges and curves.

- **Dual Filters**: Filter 1 is the main filter with cutoff (CC 74) and resonance (CC 71). Filter 2 provides additional filtering with its own cutoff (CC 55) and resonance (CC 56).

- **Advanced Envelopes**: 5 independent ADSR envelopes provide extensive modulation capabilities beyond the traditional amp and filter envelopes.

- **LFO System**: 5 LFOs each with rate and gain controls allow for complex modulation routing and polyrhythmic effects.

- **Arpeggiator**: Built-in arpeggiator with advanced features including ratcheting, chance operations, and pattern length control.

- **Effects Chain**: Pre-FX → Filters → Post-FX → Delay → Reverb signal path with independent parameter control for each stage.

- **Firmware Compatibility**: This reference is based on firmware version 1.5.5. CC assignments may vary with different firmware versions.

## Usage Examples

### Classic Analog Filter Sweep
```
CC 74 (Filter 1 Cutoff): 0 → 127
CC 71 (Filter 1 Resonance): 80
CC 52 (Filter 1 LFO1 Amount): 64
CC 72 (LFO 1 Rate): 45 (slow sweep)
```

### Wavetable Morphing Pad
```
CC 81 (ENV 1 Attack): 90 (slow attack)
CC 84 (ENV 1 Release): 100 (long release)
CC 24 (OSC 1 Wavscan): 32 (starting position)
CC 52 (Filter 1 LFO1 Amount): 32 (subtle movement)
CC 91 (Reverb Mix): 80 (ambient reverb)
```

### Mutator Lead Sound
```
CC 30 (Mutator 1 Depth): 100 (heavy processing)
CC 31 (Mutator 1 Dry/Wet): 90 (wet signal)
CC 74 (Filter 1 Cutoff): 100 (bright filter)
CC 71 (Filter 1 Resonance): 60 (moderate resonance)
CC 16 (Macro 1): Assign to Mutator Ratio for performance control
```

### Rhythmic Arpeggiator
```
CC 108 (ARP Mode): 32 (up/down pattern)
CC 106 (ARP Division): 96 (16th notes)
CC 107 (ARP Gate): 64 (staccato)
CC 109 (ARP Ratchet): 40 (subtle ratcheting)
CC 110 (ARP Chance): 100 (occasional skips)
```

This comprehensive reference provides the foundation for programming advanced sequences and real-time control of the Hydrasynth's extensive parameter set.