t = TempoClock.new(100/60).permanent_(true);
// snare from https://sccode.org/1-54H
SynthDef("snare", {arg out = 0, amp = 0.1, sinfreq = 180, att = 0.01, rel = 0.2, ffreq = 2000, pan = 0;
	var env, snd1, snd2, sum;
	env = Env.perc(att, rel, amp).kr(doneAction: 2);
	snd1 = HPF.ar(
		in: WhiteNoise.ar,
		freq: ffreq,
		mul: env
	);
	snd2 = SinOsc.ar(freq: sinfreq, mul: env);
	sum = snd1 + snd2;
	Out.ar(out, Pan2.ar(sum, pan));
}).add;
SynthDef(\sineperc, { var freq = \freq.kr(100), sig, env;
	env = EnvGen.kr(Env.perc(\atk.kr(0.01), \rel.kr(1), 1.0, [\c0.kr(4), \c1.kr(-4)]), doneAction: 2);
	freq = XLine.ar(Rand(4000, 6000), freq, 0.006);
	sig = SinOsc.ar(freq) * 0.5 * \amp.kr(1);
	sig = sig * env;
	sig = Pan2.ar(sig, \pan.kr(0));
	Out.ar(0, sig);
}).add;
~slide = 0;

(
Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70];
	loop{ var slideamp = ~slide.linlin(0.45, 0.5, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.01,
			\rel, 0.5,
			\c0, 4,
			\c1, -4,
			\amp, 0.2 * slideamp,
			\pan, rrand(-0.5, 0.5),

		]);
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70, 72];
	loop{ var slideamp = ~slide.linlin(0.5, 0.8, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.01,
			\rel, 0.5,
			\c0, 4,
			\c1, -4,
			\amp, 0.2 * slideamp,
			\pan, rrand(-0.5, 0.1),

		]);
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70, 72, 74, 75];
	loop{ var slideamp = ~slide.linlin(0.2, 0.5, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.01,
			\rel, 0.5,
			\c0, 4,
			\c1, -4,
			\amp, 0.2 * slideamp,
			\pan, rrand(-0.1, 0.5),

		]);
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);



Routine({ var i = 0, chord = [60, 60, 72]+24;
	loop{ var slideamp = ~slide.linlin(0.6, 0.85, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.001,
			\rel, 0.1,
			\c0, -4,
			\c1, -4,
			\amp, 0.3 * slideamp * (1.0/(i+1)),
			\pan, [-1, 1, 0.5, -0.5].choose,

		]);
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [24, 36, 48, 60, 72, 84];
	loop{ var slideamp = ~slide.linlin(0.9, 0.95, 0.0, 1.0);
		if(rrand(0.0, 1.0) > 0.3) {
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.001,
				\rel, 0.2,
				\c0, -2,
				\c1, -4,
				\amp, 0.3 * slideamp,
				\pan, [0.5, -0.5, 0.25, -0.25, 0].choose,

			]);
			i = (i + 1)%chord.size;
		};
		0.2.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [60, 60, 60, 72, 72]+36;
	loop{ var slideamp = ~slide.linlin(0.8, 0.85, 0.0, 1.0);
		if( i != (chord.size-1)) {
			Synth(\sineperc, [
				\freq, chord[i].midicps,
				\atk, 0.001,
				\rel, 0.1,
				\c0, -4,
				\c1, -4,
				\amp, 0.3 * slideamp * ((i+1)/chord.size),
				\pan, [-1, 1, 0.5, -0.5].choose,

			]);
		};
		i = (i + 1)%chord.size;
		0.2.wait;
	};
}).play(t, 4);



// chord
Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70, 72, 74, 75];
	loop{ var slideamp = ~slide.linlin(0.4, 0.45, 1.0, 0.0);
		if(rrand(0.0, 1.0) > 0.4) {
			3.do {
				Synth(\sineperc, [
					\freq, chord.choose.midicps,
					\atk, rrand(0.6, 0.9),
					\rel, 2,
					\c0, 4,
					\c1, -4,
					\amp, 0.2 * slideamp,
					\pan, rrand(-0.1, 0.5),
				]);
			};
		};
		1.2.wait;
	};
}).play(t, 4);



Routine({ var i = 0, chord = [60, 65, 67, 68, 70, 72, 75]-12;
	loop{ var slideamp = ~slide.linlin(0.4, 0.45, 0.0, 1.0) - ~slide.linlin(0.6, 0.7, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.001,
			\rel, 0.2,
			\c0, -4,
			\c1, -4,
			\amp, 0.25 * slideamp,
			\pan, rrand(-0.5, 0.5),

		]);
		i = (i + 1)%chord.size;
		0.4.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [60, 65, 67, 68, 72]-12;
	loop{ var slideamp = ~slide.linlin(0.25, 0.3, 0.0, 1.0) - ~slide.linlin(0.5, 0.6, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.001,
			\rel, 0.2,
			\c0, -4,
			\c1, -4,
			\amp, 0.25 * slideamp,
			\pan, rrand(-0.5, 0.5),

		]);
		i = (i + 1)%chord.size;
		0.6.wait;
	};
}).play(t, 4);

Routine({ var i = 0, chord = [60, 65, 67, 68, 72]-12;
	loop{ var slideamp = ~slide.linlin(0.15, 0.2, 0.0, 1.0) - ~slide.linlin(0.3, 0.35, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.001,
			\rel, 0.2,
			\c0, -4,
			\c1, -4,
			\amp, 0.25 * slideamp,
			\pan, rrand(-0.5, 0.5),

		]);
		i = (i + 1)%chord.size;
		1.0.wait;
	};
}).play(t, 4);



Routine({ var i = 0, chord = [60, 65, 67, 68, 70, 72, 75]-24;
	loop{ var slideamp = ~slide.linlin(0.5, 0.55, 0.0, 1.0) - ~slide.linlin(0.65, 0.7, 0.0, 1.0);
		Synth(\sineperc, [
			\freq, chord[i].midicps,
			\atk, 0.1,
			\rel, 1,
			\c0, -4,
			\c1, -4,
			\amp, 0.8 * slideamp,
			\pan, rrand(-0.5, 0.5),
		]);
		i = (i + 1)%chord.size;
		[0.6, 0.4, 1.2, 0.8].choose.wait;
	};
}).play(t, 0.5);


Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-36;
	loop{ var slideamp = ~slide.linlin(0.05, 0.15, 0.0, 1.0) * ~slide.linlin(0.3, 0.35, 1.0, 0.0);
		Synth(\sineperc, [
			\freq, chord.choose.midicps,
			\atk, 0.1,
			\rel, 2,
			\c0, -4,
			\c1, -6,
			\amp, 0.8 * slideamp,
			\pan, rrand(-0.1, 0.1),
		]);
		i = (i + 1)%chord.size;
		2.4.wait;
	};
}).play(t, 0.5);

/*Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-24;
	loop{ var slideamp = ~slide.linlin(0.85, 0.9, 0.0, 1.0);
		if(rrand(0.0, 1.0) > 0.6) {
		Synth(\sineperc, [
			\freq, chord.choose.midicps,
			\atk, 0.001,
			\rel, 6,
			\c0, -4,
			\c1, -8,
			\amp, rrand(0.3, 0.8) * slideamp,
			\pan, rrand(-0.1, 0.1),
		]);
		i = (i + 1)%chord.size;
		};
		1.2.wait;
	};
}).play(t, 1);*/

Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-24;
	loop{ var slideamp = ~slide.linlin(0.85, 0.9, 0.0, 1.0);
		if(rrand(0.0, 1.0) > 0.6) {
		Synth(\sineperc, [
			\freq, chord.choose.midicps,
			\atk, 0.001,
			\rel, 3,
			\c0, -4,
			\c1, -8,
			\amp, rrand(0.3, 0.8) * slideamp,
			\pan, rrand(-0.1, 0.1),
		]);
		i = (i + 1)%chord.size;
		};
		0.2.wait;
	};
}).play(t, 1);


 // snare from https://sccode.org/1-54H
~snareRout = Routine({ var i = 0, chord = [60, 67, 68, 72, 75, 77, 79]-36, waittime;
	loop{ var slideamp = ~slide.linlin(0.3, 0.4, 0.0, 1.0) - ~slide.linlin(0.6, 0.7, 0.0, 1.0);
		Synth(\snare, [
			\att, 0.01,
			\rel, 0.1,
			\sinfreq, 180,
			\ffreq, 2000,
			\amp, rrand(0.05, 0.15) * slideamp,
		]);
		if(waittime == 0.1, {
			waittime = [0.3].choose;
		}, {
			waittime = [0.8, 0.4, 1.2, 0.1].choose;
		});

		waittime.wait;
	}
});
// ~snareRout.play(t, 1);
// ~snareRout.play(t, 1);

// bright chord
Routine({ var i = 0, chord = [60, 63, 65, 67, 68, 70, 72, 74, 75]+12;
	loop{ var slideamp = ~slide.linlin(0.5, 0.6, 0.0, 1.0) - ~slide.linlin(0.8, 0.85, 0.0, 1.0);
		if(rrand(0.0, 1.0) > 0.4) {
			3.do {
				Synth(\sineperc, [
					\freq, chord.choose.midicps,
					\atk, rrand(1.2, 2.9),
					\rel, 2,
					\c0, 4,
					\c1, -4,
					\amp, 0.2 * slideamp,
					\pan, rrand(-0.1, 0.5),
				]);
			};
		};
		2.4.wait;
	};
}).play(t, 4);
)
( // change the bounds to become vertical
w = Window.new.front;
c = NumberBox(w, Rect(20, 20, 150, 20));
a = Slider(w, Rect(200, 60, 20, 150))
.action_({
	c.value_(a.value);
	~slide = a.value;
});
a.action.value;
)
(
Routine({
	loop {

		{ a.valueAction_(rrand(0.0, 1.0)); }.defer;
		0.4.wait;
	}
}).play(t, 1);
)
(
Routine({ var val = 0.0, v1 = 0.05, v2 = 0.025;
	loop {

		{ a.valueAction_(val); }.defer;
		val = val + v1;
		if(val > 1.0) { var temp = v1; v1 = v2; v2 = temp; };
		0.4.wait;
		{ a.valueAction_(val); }.defer;
		val = val - v2;
		0.4.wait;
	}
}).play(t, 1);
)