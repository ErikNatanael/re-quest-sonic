~functionCallTimestamps = List[];
(
OSCdef(\function_call_envelope, {
	// receive a transmission from OF of all the timestamps of all function calls as one message
	|msg, time, addr, recvPort|
	var script, index, env;
	msg.postln;
	~functionCallTimestamps = msg[1..];
	~functionCallTimestamps.size.postln;
	//
	env = ~averageFunctionCallDensity.(~functionCallTimestamps, 1024);
},
'/functionCallTransmission', n);

~averageFunctionCallDensity = { | timestamps, numPoints |
	var windowSize = 1.0/numPoints, threshold = windowSize, envelope = 0!numPoints, index = 0;
	timestamps.do{ |ts|
		if(ts > threshold) {
			// go to the next window and start counting
			threshold = threshold + windowSize;
			index = index + 1;
		};
		envelope[index] = envelope[index] + 1;
	};
	envelope // return the envelope
};
)

~functionCallTimestamps.plot
~averageFunctionCallDensity.(~functionCallTimestamps, 16).plot

(// PLAY THE SLIDER
Routine({var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 64).sqrt.sqrt/5.0,
	lastValue = 0, change = 0;
	activity = activity.add(0); // go back to 0 at the end
	activity.do { | val |
		change = val - lastValue;
		10.do {
			lastValue = lastValue + (change*0.1);
			{ a.valueAction_(lastValue); }.defer; // set slider value
			0.1.wait;
		}
	}
}).play(t, 1);
)

// MUSICAL PIECE
// Individual processes use the envelope for amplitude, subdividing it over different lengths of time.
(
Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-24;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 16).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.001,
				\rel, 3,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, rrand(-0.1, 0.1),
			]);
			i = (i + 1)%chord.size;
			0.2.wait;
		}
	};
}).play(t, 1);
)
(
Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-12;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 32).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.01,
				\rel, 1,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, rrand(-0.1, 0.1),
			]);
			i = (i + 1)%chord.size;
			0.4.wait;
		}
	};
}).play(t, 1);
)

(
Routine({ var i = 0, chord = [60, 60, 72]+12;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 64).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.01,
				\rel, 1,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, rrand(-1.0, 1.0),
			]);
			i = (i + 1)%chord.size;
			0.4.wait;
		}
	};
}).play(t, 1);
)

(
Routine({ var i = 0, chord = [63, 68, 75]+12;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 128).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.01,
				\rel, 2,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, rrand(-1.0, 1.0),
			]);
			i = (i + 1)%chord.size;
			0.4.wait;
		}
	};
}).play(t, 1);
)

(
Routine({ var i = 0, chord = [67+12, 67+24, 67+36, 67+24, 67+36, 67+24]+12;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 512).normalize.pow(2);

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.0001,
				\rel, 0.1,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, [-1.0, 1.0].choose,
			]);
			i = (i + 1)%chord.size;
			0.2.wait;
		}
	};
}).play(t, 1);
)
(
Routine({ var i = 0, chord = [36, 48];
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 16).normalize.pow(2);

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.0001,
				\rel, 0.1,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, [-0.1, 0.1].choose,
			]);
			i = (i + 1)%chord.size;
			0.1.wait;
		}
	};
}).play(t, 1);
)
(
Routine({ var i = 0, chord = ({[36, 48]}!5 ++ [24]).flatten;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 512).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.0001,
				\rel, 5*activityVal,
				\c0, -4,
				\c1, -8,
				\amp, activityVal * slideamp,
				\pan, [-0.1, 0.1].choose,
			]);
			i = (i + 1)%chord.size;
			0.1.wait;
		}
	};
}).play(t, 1);
)