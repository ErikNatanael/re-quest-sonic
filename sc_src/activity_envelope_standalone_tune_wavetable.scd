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
~wt_buf[0].plot


// CREATE WAVETABLES FROM THE ACTIVITY PLOT
( var numTables = 5;
~wt_sig = numTables.collect({
	arg i;
	var activity, numSegs;
	activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 2.pow(i+4)).normalize;
	numSegs = activity.size+1;

	Env(
		//activity goes
		[0]++activity++[0],

		//every segment is the same length
		{1}.dup(numSegs),

		//low-index wavetables tend to be sinusoidal
		//high index wavetables tend to have sharp angles and corners
		{[\sine,0,exprand(1,20) * [1,-1].choose].wchoose([9-i,3,i].normalizeSum)}.dup(numSegs)
	).asSignal(1024);
});

//load into 10 buffers in wavetable format
~wt_buf = Buffer.allocConsecutive(numTables, s, 2048, 1, {
	arg buf, index;
	buf.setnMsg(0, ~wt_sig[index].asWavetable);
});

SynthDef(\osc, {
	arg buf=0, freq=200, detune=0.2,
	amp=0.2, pan=0, out=0, rout=0, rsend=(-20),
	atk=0.01, sus=1, rel=0.01, c0=1, c1=(-1);
	var sig, env, detuneCtrl;
	env = EnvGen.ar(
		Env([0,1,1,0],[atk,sus,rel],[c0,0,c1]),
		doneAction:2
	);

	//array of eight Oscs with uniquely detune frequencies
	//and unique initial phase offsets
	detuneCtrl = LFNoise1.kr(0.1!8).bipolar(detune).midiratio;
	sig = Osc.ar(buf, freq * detuneCtrl, {Rand(0,2pi)}!8);

	sig = Splay.ar(sig); //spread 8 signals over stereo field
	sig = LeakDC.ar(sig); //remove DC bias
	sig = Balance2.ar(sig[0], sig[1], pan, amp); //L/R balance (pan)
	sig = sig * env;
	Out.ar(out, sig);
	Out.ar(rout, sig * rsend.dbamp); //"post-fader" send to reverb
}).add;
)

(
Pbind(\instrument, \osc,
	\dur, Pseq([1/12], inf),
	\freq, Pseq([60, 67, 68, 72, 75, 77, 79].midicps, inf),
	\atk, Pwhite(0.001, 0.01),
	\sus, 0,
	\rel, Pwhite(0.05, 0.4),
	\c0, exprand(3, 6).neg,
	\c1, -5,
	\buf, Pstutter(8, Pseq(~wt_buf, inf)),
	\amp, Pwhite(0.01, 0.1),
	\pan, Pseq([1,-1],inf),
	\out, 0,
	\rout, ~rbus,
	\rsend, 10,
).play;
)

// MUSICAL PIECE
// Individual processes use the envelope for amplitude, subdividing it over different lengths of time.
(
Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-24;
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 16).normalize;

		activity.do { | activityVal |
			var slideamp = 1;
			Synth(\osc, [
				\freq, chord[i].midicps,
				\atk, 0.001,
				\sus, 0,
				\rel, 3,
				\c0, -4,
				\c1, -8,
				\buf, ~wt_buf[0],
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
Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70];
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 32).normalize;
		activity.do{ | activityVal |
			Synth(\osc, [
				\freq, chord[i].midicps,
				\atk, 0.01,
				\sus, 0,
				\rel, 0.5,
				\c0, 4,
				\c1, -4,
				\buf, ~wt_buf[1],
				\amp, 0.2 * activityVal,
				\pan, rrand(-0.5, 0.5),

			]);
			i = (i + 1)%chord.size;
			0.2.wait;
		};
	};
}).play(t, 4);
)
(
Routine({ var i = 0, chord = [60, 60, 72]+24;
	loop{ var slideamp = 1;
		Synth(\osc, [
			\freq, chord[i].midicps,
			\atk, 0.001,
			\sus, 0,
			\rel, 0.1,
			\c0, -4,
			\c1, -4,
			\buf, ~wt_buf[4],
			\amp, 0.3 * slideamp * (1.0/(i+1)),
			\pan, [-1, 1, 0.5, -0.5].choose,

		]);
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);
)

( // chord
Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70, 72, 74, 75];
	loop{ var activity = ~averageFunctionCallDensity.(~functionCallTimestamps, 32).normalize;
		activity.do{ | activityVal |
			3.do {
				Synth(\osc, [
					\freq, chord.choose.midicps,
					\atk, rrand(0.6, 0.9),
					\sus, 0,
					\rel, 2,
					\c0, 4,
					\c1, -4,
					\buf, ~wt_buf.choose,
					\amp, 0.2 * activityVal,
					\pan, rrand(-0.1, 0.5),
				]);
			};
			1.2.wait;
		};
	};
}).play(t, 4);
)
