

(
s.waitForBoot({
	~wt_buf.do(_.free);
	//10 wavetables with increasing complexitymovementRecord
	~wt_sig = 10.collect({
		arg i;

		//random number of envelope segments
		var numSegs = i.linexp(0,9,4,40).round;

		Env(
			//env always begins and ends with zero
			//inner points are random from -1.0 to 1.0
			[0]++({1.0.rand}.dup(numSegs-1) * [1,-1]).scramble++[0],

			//greater segment duration variety in higher-index wavetables
			{exprand(1,i.linexp(0,9,1,50))}.dup(numSegs),

			//low-index wavetables tend to be sinusoidal
			//high index wavetables tend to have sharp angles and corners
			{[\sine,0,exprand(1,20) * [1,-1].choose].wchoose([9-i,3,i].normalizeSum)}.dup(numSegs)
		).asSignal(1024);
	});

	//load into 10 buffers in wavetable format
	~wt_buf = Buffer.allocConsecutive(10, s, 2048, 1, {
		arg buf, index;
		buf.setnMsg(0, ~wt_sig[index].asWavetable);
	});
	s.sync();

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
		sig = sig * AmpCompA.kr(freq, 20);
		sig = Balance2.ar(sig[0], sig[1], pan, amp); //L/R balance (pan)
		sig = sig * env;
		Out.ar(out, sig);
		Out.ar(rout, sig * rsend.dbamp); //"post-fader" send to reverb
	}).add;
	SynthDef(\noisepad, { |out = 0, amp = 0.5, freq = 440, gate = 1, attack = 0.01, decay = 0.1, roughness = 0.01, pressure = 0.3, pan = 0, freqLag = 0.2, fragAmount = 0.7, fragDensity = 10, focus = 0.01, maxRoughness = 1, minRoughness = 0, lpf = 10000 |

		var sig, env, unison, detune, freqs, noise, osc2, dist, fragmentTrigger, distSig, envtrig;
		unison = 7;
		detune = 0.03;
		detune = \detune.kr(0.001);//LFNoise2.kr(1/2).range(0.001, 0.05);
		dist = LFNoise1.kr(1).range(\minDist.kr(0), \maxDist.kr(1));
		roughness = LFNoise1.kr(0.2).range(minRoughness, maxRoughness);

		// add an attack every time the frequency changes
		envtrig = Changed.kr(freq);
		env = EnvGen.kr(Env(
			levels: [0, 1, 0.2, 0.0],
			times: [0.001, 1, 0.5]),
		gate: envtrig + Impulse.kr(0)
		);

		freq = freq.lag(freqLag);
		pan = pan.lag(1);

		freqs = unison.collect{|id| freq * (1 + detune*(id+1)) };
		//freqs = freqs.addAll(freq*[2, 4, 5, 6]);
		//freqs.poll;

		/*env = EnvGen.kr(Env(
		levels: [0, 1, 0.5, 0],
		times: [attack, 0.8, decay],
		curve: [0, 0, -7],
		releaseNode: 2),
		gate: gate,
		doneAction: 2);*/

		sig = Resonz.ar(BrownNoise.ar(), freqs, 0.001) * 2;

		//sig = RLPFD.ar(sig, freqs, 0.4, 0.1) * 3;
		sig = Mix(sig);
		sig = CombL.ar(sig, 0.2, freq.reciprocal, -3);

		osc2 = Resonz.ar(WhiteNoise.ar(), freq, 0.001) * 0.0001;
		osc2 = RLPF.ar(RHPF.ar(WhiteNoise.ar()*0.01, freqs, focus*2), freq, focus);
		osc2 = Mix(osc2);

		sig = (sig * roughness) +  (osc2 * (1 - roughness));

		distSig = (DFM1.ar(sig, freq, dist, 1, 0.0, 0.005*dist) * (20 + (dist*20))).tanh*0.3;
		sig = (sig * (1-dist)) + (distSig * dist);
		//sig = sig + (DFM1.ar(sig, 100, 1.0, 20*rough).tanh);

		fragmentTrigger = Changed.kr(\fragmentTrigger.kr(1), 0.1);
		// fragmentTrigger = Changed.kr(LFNoise0.kr(fragDensity), 0.4);
		sig = (sig*(1-fragAmount)) + (sig * Lag.kr(Demand.kr(fragmentTrigger + Impulse.kr(0), 0, Dseq([1, 0], inf)), 0.02) * fragAmount);
		sig = LPF.ar(sig, lpf.min(19000)) * AmpCompA.kr(freq, 20);


		Out.ar(out, Pan2.ar(sig, pan)*env*amp);
	}).add;
	SynthDef(\windy, { |out = 0, knob = 0.5, decay = 10, gate = 1, amp = 1|
		var osc = BrownNoise.ar(0.5), env, focus, outs, freqs;
		env = EnvGen.kr(Env(
			levels: [0, 1, 0],
			times: [1, decay],
			releaseNode: 1),
		gate: gate,
		doneAction: 2);
		knob = knob*LFNoise1.kr(0.6).range(0.95, 1.1);
		freqs = [Lag.kr(knob, 0.4) * [2000, 2020] + 100, Lag.kr(knob, 0.05)*[100, 102]+60, Lag.kr(knob, 1.0)*500+40, Lag.kr(knob, 5)*300+30];
		focus = [LFNoise1.kr(4).range(0.001, 0.001), LFNoise2.kr(1.1).range(0.0001, 0.01), LFNoise2.kr(0.2).range(0.001, 0.05)];
		outs = Resonz.ar(osc, freqs, focus);
		outs = Mix.ar(outs);

		outs = (outs*100).tanh * 0.1;
		outs = HPF.ar(outs, \cutoff.kr(500));
		outs = outs * env*amp;
		outs = outs + PitchShift.ar(outs, 0.2, 1.0, 0.05, 0.01);

		Out.ar(out, [outs, DelayN.ar(outs, 0.01, 0.01)]);
	}).add;
	SynthDef(\swoosh, {
		var sig, env, freq, clap, popEnv, centerFreq = Rand(200, 500)*3, length = \length.kr(1), release = Rand(0.4, 0.7)*2 * length;
		env = EnvGen.ar(
			Env(
				levels: [0, 1, 0],
				times: [length, release],
				curve: [-2, -2]
			),
			doneAction:2
		);
		popEnv = EnvGen.ar(Env([3261, 410, Rand(30, 80)], [0.002, 0.01], curve: \exp));

		freq = [
			EnvGen.ar(Env([centerFreq*Rand(0.5, 1.2), centerFreq], [length], [Rand(4, 7)])),
			EnvGen.ar(Env([centerFreq*Rand(0.4, 0.8), centerFreq], [length], [Rand(2, 4)]))];
		freq = freq * LFNoise1.kr(Line.kr(1, Rand(3, 20), 2)).range(0.8, 1.1);
		sig = Resonz.ar(WhiteNoise.ar(), freq, Line.kr(Rand(0.3, 0.8), Rand(0.05, 0.8), length + release))*0.5;
		sig = HPF.ar(sig, Rand(500, 100));
		sig = LPF.ar(sig, Rand(5000, 12000));
		sig = Mix(sig);
		sig = Pan2.ar(sig, Line.kr(Rand(-1, 1), Rand(-1, 1), length+1));
		sig = sig * env * 0.5 * \amp.kr(0.3);
		//sig = FreeVerb.ar(sig, 0.05);
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\friction_swoosh, {
		var sig, env, freq, clap, popEnv, centerFreq = Rand(200, 500)*3, length = \length.kr(1), release = Rand(0.4, 0.7)*2 * length, frictionMod, friction, mass;
		env = EnvGen.ar(
			Env(
				levels: [0, 1, 0],
				times: [length, release],
				curve: [-2, -2]
			),
			doneAction:2
		);
		popEnv = EnvGen.ar(Env([3261, 410, Rand(30, 80)], [0.002, 0.01], curve: \exp));

		freq = [
			EnvGen.ar(Env([centerFreq*Rand(0.5, 1.2), centerFreq], [length], [Rand(4, 7)])),
			EnvGen.ar(Env([centerFreq*Rand(0.4, 0.8), centerFreq], [length], [Rand(2, 4)]))];
		freq = freq * LFNoise1.kr(Line.kr(1, Rand(3, 20), 2)).range(0.8, 1.1);
		sig = Resonz.ar(WhiteNoise.ar(), freq, Line.kr(Rand(0.3, 0.8), Rand(0.05, 0.8), length + release))*0.5;
		sig = HPF.ar(sig, Rand(500, 100));
		sig = LPF.ar(sig, Rand(5000, 12000));
		sig = Mix(sig);
		// add some friction
		frictionMod = LFNoise1.kr(1).range(0, 1);
		// nice friction values = 5.37133e-05 - 0.0152347
		// friction =  MouseX.kr(0.00001, 0.03, 1);
		friction = frictionMod.linlin(0, 1, 5.37133e-06, 0.0152347);
		//nice mass values = 0.7 - 0.411227
		// mass = MouseY.kr(0.2, 10, 1);
		mass = frictionMod.linlin(0, 1, 0.5, 0.311227);
		sig = Friction.ar(sig, friction:friction, mass: mass);
		sig = Pan2.ar(sig, Line.kr(Rand(-1, 1), Rand(-1, 1), length+1));
		sig = sig * env * 0.5 * \amp.kr(0.3);

		//sig = FreeVerb.ar(sig, 0.05);
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\tick, {
		var sig, env;
		env = Env.perc(0, 0.2).kr(2);
		sig = Impulse.ar(0);
		sig = sig * 0.1;
		sig = RLPF.ar(sig, \freq.kr(1000), \rq.kr(0.1));
		sig = Pan2.ar(sig, \pan.kr(0)) * \amp.kr(1.0);
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\sineperc, { var freq = \freq.kr(100), sig, env;
		env = EnvGen.kr(Env.perc(\atk.kr(0.01), \rel.kr(1), 1.0, [\c0.kr(4), \c1.kr(-4)]), doneAction: 2);
		freq = XLine.ar(Rand(4000, 6000), freq, 0.006);
		sig = SinOsc.ar(freq) * 0.5 * \amp.kr(1);
		sig = sig * env;
		sig = Pan2.ar(sig, \pan.kr(0));
		Out.ar(0, sig);
	}).add;
	SynthDef(\sineperc_retrig, { var freq = \freq.kr(100), sig, env, inTrig = InTrig.kr(\trigBus.kr(10)), freqEnv, sawEnv, fmEnv;
		env = EnvGen.kr(
			Env.perc(\atk.kr(0.01), \rel.kr(1), 1.0, [\c0.kr(4), \c1.kr(-4)]),
			gate: inTrig
			// gate: \gate.kr(0)
		);
		freq = freq * LFNoise1.kr(10).range(0.998, 1.002); // TODO: Less vibrato for high notes, more for low
		freqEnv = EnvGen.ar(Env([TRand.ar(4000, 6000, inTrig), freq], [0.006]), gate: inTrig);
		sawEnv = EnvGen.ar(Env([1, 1, 0.5, 0], [0.001, 0.01, 0.5]), gate: inTrig);
		fmEnv = EnvGen.ar(Env([freq*6, freq*6, freq], [0.00001, 0.01]), gate: inTrig);
		sig = SinOsc.ar(freqEnv + SinOsc.ar(freq*2, 0, fmEnv));
		sig = sig + (SinOsc.ar(freq*0.5, 0, 0.4));
		// sig = sig + (Saw.ar(freq*2.01) * sawEnv * 0.2);
		sig = sig * 0.8 * \amp.kr(1);
		sig = sig * AmpCompA.kr(freq, 20, 0.32, 2.0);
		sig = sig * env;
		sig = Pan2.ar(sig, \pan.kr(0));
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\filtered_noise, {
		var sig, env, freqs, abortEnv;
		env = EnvGen.kr(
			Env(
				levels: [0, 1, 0],
				times: [\atk.kr(0.5), \rel.kr(4)],
				curve: [\c0.kr(2), \c1.kr(-2)]),
			doneAction: 2
		);
		abortEnv = EnvGen.kr(
			Env(
				levels: [1, 0],
				times: [0.5],
				curve: [-2],
				releaseNode: 0,
			),
			gate: \gate.kr(1) + Impulse.kr(0), // the impulse is needed in case the gate is set within the same control period
			doneAction: 2
		);
		sig = PinkNoise.ar();
		sig = Resonz.ar(sig*16, \freq.kr(400), 0.001)*env*abortEnv;
		sig = Pan2.ar(sig, \pan.kr(0.0));
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\verb, {
		var snd, wet, feedback, local, pitchedUp;
		snd = In.ar(\out.kr(0), 2);
		pitchedUp = PitchShift.ar(snd, 0.2, 2);
		wet = snd.sum + pitchedUp;
		wet = DelayN.ar(wet, 0.02, 0.02);
		wet = JPverb.ar(wet, 4.0, 0.0, 1.8);
		feedback = LocalIn.ar(2);
		wet = HPF.ar(wet, 100) * -3.dbamp;
		wet = LPF.ar(wet, 7000);
		snd = snd + (wet * \wetness.kr(0.1));
		// some extra (potentially infinite) reverb
		local = snd;
		15.do{local = AllpassN.ar(snd + feedback, 0.06, Rand(0.001, 0.06), 1);};
		// local = local + AllpassN.ar(pitchedUp, 0.3, 0.3, 1.0);
		LocalOut.ar(local * \feedback.kr(0.1));
		snd = snd + ((feedback*10).tanh * 0.5);
		// Add a little bit of tape flutter
		snd = DelayC.ar(snd, 0.2, LFNoise2.kr(1).range(0, 0.005));

		//snd = (snd * Demand.kr(Dust.kr(5), 1, Dseq([1, 0], inf))) + (snd*0.5);

		ReplaceOut.ar(\out.kr(0), snd);
	}).add;
	SynthDef(\tick2, {
		var sig, env, rq = \rq.kr(0.1);
		env = Env.perc(0, 0.02).ar(2);
		sig = Impulse.ar(0);
		sig = sig * 0.5;
		sig = RHPF.ar(sig, \freq.kr(1000), rq) * env;
		sig = Pan2.ar(sig, \pan.kr(0)) * \amp.kr(1.0);
		Out.ar(\out.kr(0), sig);
	}).add;
	SynthDef(\master_effect, {
		var sig, env, amp = \amp.kr(1.0), peak = 1.0 - PeakFollower.kr((1.0 - amp).lag(1.0));
		var electric_sig, electric_trigs, electric_freqs;

		var resonator_sig;
		var a, k, delfreq, minfreqs, freqs, dry;
		// get the signal of everything
		sig = In.ar(\in.kr(16), 2);
		sig = sig + (In.ar(\functionCallIn.kr(18), 2) * \functionCallAmp.kr(1.0, 0.4));

		// electrical dust start
		electric_freqs = LFNoise1.kr(1/2).range(500, 10000) * [0.7, 0.6, 0.8, 0.9, 0.95, 1.0];
		electric_trigs = {Dust.ar(Rand(10, 40))}!6;
		electric_trigs = Decay.ar(electric_trigs, LFNoise1.kr(1.2).range(0, 1).linexp(0, 1, 0.00001, 0.01));
		electric_sig = SinOsc.ar(electric_freqs) * electric_trigs * 0.1;
		electric_sig = Splay.ar(electric_sig) * \dustAmp.kr(0.0);
		// electrical dust end

		// resonator start
		/*resonator_sig = LocalIn.ar(2) + sig;
		resonator_sig = resonator_sig + WhiteNoise.ar(0.001!2);

		// delay due to distance from amp - I chose 0.05s, or 20Hz
		delfreq = 10;
		resonator_sig = DelayN.ar(resonator_sig,1/10-ControlDur.ir,1/delfreq-ControlDur.ir);

		// guitar string frequencies - for some reason I had to pitch them down
		// a few octaves to get a good sound.
		// open strings
		freqs = (64+[0,12, 24, 19, 28]).midicps/8;
		freqs = freqs * LFNoise0.kr((peak * 3.0) + 0.1).range(0.9, 1.1);
		// e minor
		// freqs = (64+[0,7,12,15,19,24]).midicps/4;

		// whammy bar modulates freqs:
		minfreqs = freqs*0.5;
		freqs = freqs*MouseButton.kr(1,0.75,4);

		// 6 comb filters emulate the strings' resonances
		resonator_sig = CombN.ar(resonator_sig!6,1/minfreqs,1/freqs,8).mean;

		// a little filtering... amp controls amp
		resonator_sig = LPF.ar(resonator_sig,8000);
		resonator_sig = HPF.ar(resonator_sig*amp.lag(1).linexp(0, 1, 2.0, 5.0),80);

		// and some not too harsh distortion - peak controls damping
		resonator_sig = RLPFD.ar(resonator_sig, peak.linexp(0.0001, 1.0, 200, 10000)*[1,1.1],0.1,0.5);
		resonator_sig = resonator_sig + resonator_sig.mean;

		// resonator_sig = (resonator_sig * Demand.kr(Dust.kr(35), 1, Dseq([1, 0], inf)));
		resonator_sig = resonator_sig + SinOsc.ar([LFNoise0.kr(10).range(500, 2000), LFNoise0.kr(15).range(500, 2000)]) * 0.5;*/
		// and finally a spot of reverb
		/*dry = resonator_sig;
		5.do {
		d = 0.05.rand;
		resonator_sig = AllpassN.ar(resonator_sig,d,d,5);
		};
		resonator_sig = (dry + resonator_sig)*0.125;*/
		/*LocalOut.ar(resonator_sig, 2);
		resonator_sig = resonator_sig * 0.2 + (PitchShift.ar(resonator_sig, 0.2, 2.0, LFNoise1.kr(6).range(0.01, 0.05), 0.05) * 0.8);
		resonator_sig = resonator_sig * amp;*/
		// resonator end
		sig = sig + electric_sig;
		sig = sig * PeakFollower.kr(amp.linlin(0.0, 0.2, 0.0, 1.0), 0.99);
		// sig = (sig * amp) + (HPF.ar(sig, amp.lag(0.1).linexp(0.0, 1.0, 5000, 200)) * (1.0-amp));
		// sig = HPF.ar(sig, amp.lag(0.1).linexp(0.0, 1.0, 5000, 20));
		/*sig = sig + Limiter.ar(resonator_sig * 0.01, 0.01);*/
		sig = PitchShift.ar(sig, 0.2, 0.5 + PeakFollower.kr(amp.linlin(0.0, 0.05, 0.0, 0.5), 0.99999).lag(0.5), max(0.1 - amp.lag(0.3), 0));
		Out.ar(\out.kr(0), sig);
	}).add;

	~clickSample = Buffer.readChannel(s, "../samples/442649__asetkeyzet__keyboard-click_single.wav".resolveRelative, channels: [0]);
	s.sync();
	//cleanup
	Pdef.all.do(_.clear);
	Pbindef.all.do(_.clear);
	~ratio53 = pow(2, 1/53);
	~rootFreq = 0.midicps;
	// Function to convert a degree in 53-EDO to a frequency
	~degree53_to_freq =  { |degree|
		var freq = ~rootFreq*pow(~ratio53, degree);
		freq
	};
	~findClosestVoicing = { | currentChord, newDegrees, edo = 53, minNote = -22, maxNote = 106 |
		var newChord;
		newChord = newDegrees.collect { | degree, i |
			var minDistance = 1000, minDistIndex = 0, minDistOctave, newNote;
			// find the note in the current chord that is closest to the new note from above or below
			currentChord.do { | note, i | var distance = 10000, old, new, oldOctave;
				// In SuperCollider -22%53 == 31 so % works for negative numbers (wrapping them to positive)
				// check octave of the chord note. The octave is stored as the offset in degrees, not number of octaves
				oldOctave = note - (note%edo);
				// bring both notes into the lowest octave
				old = note%edo;
				new = degree%edo;
				// check distan  ce
				distance = abs(new-old);
				if(distance < minDistance) {
					minDistance = distance;
					minDistOctave = oldOctave;
					minDistIndex = i;
				};

				// reverse the order of the notes
				if(old < new, {
					// bring new note down an octave
					new = new - edo;
					oldOctave = oldOctave - edo;
				}, {
					// bring new note up an octave
					new = new + edo;
					oldOctave = oldOctave + edo;
				});

				// check distance again in the reversed interval
				distance = abs(new-old);
				if(distance < minDistance) {
					minDistance = distance;
					minDistOctave = oldOctave;
					minDistIndex = i;
				};
			};
			// return the new adjusted note
			newNote = (degree%edo) + minDistOctave;
			if(newNote > maxNote, {
				newNote = newNote - edo;
			});
			if(newNote < minNote, {
				newNote = newNote + edo;
			});
			newNote
		};
		newChord
	};


	~rbus = Bus.audio(s,2);
	~masterBus = Bus.audio(s, 2);
	~functionCallBus = Bus.audio(s, 2);
	s.sync();
	~verbSynth = Synth(\verb, [\wetness, 0.3]);
	~master = Synth(\master_effect, [\in, ~masterBus, \functionCallIn, ~functionCallBus, \out, 0, \amp, 1.0]);

	~functionCallDensity = 0;
	~loadEventDensity = 0;
	~loadEventLastTime = 0;
	~loadEventOctave = 0;
	~timeScale = 1;
	~currentRoot = 31;
	~currentDegrees = [0, 17 + 53, 31, 47 + 53];
	~currentChord = ~currentDegrees + ~currentRoot;
	~filteredNoiseSynths = [];
	~scriptChords = [];
	~scriptInfo = ();
	~doPlayFunctionCalls = true;

	~generateChords = { | offset = 0 |
		var lastNote;
		/*~spectrumChord = 32.collect{|i| var newNote;
		if(i == 0, {
		newNote = 0;
		}, {
		newNote = lastNote + switch(i%5,
		1, {9},
		2, {5},
		3, {17},
		4, {9},
		0, {13}
		);
		});
		lastNote = newNote;
		newNote + 12
		};*/
		~spectrumChord = 24.collect{|i| var newNote;
			// maj7 chord
			if(i == 0, {
				newNote = 0;
			}, {
				newNote = lastNote + switch(i%4,
					1, {17},
					2, {14},
					3, {17},
					0, {5}
				);
			});
			lastNote = newNote;
			newNote + offset
		};

		lastNote = 0;

		~builtinSpectrumChord = /*12.collect{|i| var newNote;
		if(i == 0, {
		newNote = 0;
		}, {
		newNote = lastNote + switch(i%2,
		1, {31},
		0, {22},
		);
		});
		lastNote = newNote;
		newNote
		};*/
		24.collect{|i| var newNote;
			// major 6/9 chord
			if(i == 0, {
				newNote = 0;
			}, {
				newNote = lastNote + switch(i%5,
					1, {9},
					2, {8},
					3, {14},
					4, {9},
					0, {13}
				);
			});
			lastNote = newNote;
			newNote + 53 + offset
		};
	};
	~generateChords.value();

	~spectrumTriggers = ~spectrumChord.collect{|degree, i| Bus.control(s, 1);};
	~builtinSpectrumTriggers = ~builtinSpectrumChord.collect{|degree, i| Bus.control(s, 1);};
	~spectrumSynths = ~spectrumChord.collect{|degree, i|
		var rel = max(5 - (i/2), 2);
		Synth(\sineperc_retrig, [\freq, ~degree53_to_freq.value(degree + (53*3)), \amp, 0.1, \trigBus, ~spectrumTriggers[i], \rel, rel, \out, ~masterBus]);
	};
	~builtinSpectrumSynths = ~builtinSpectrumChord.collect{|degree, i|
		var rel = max(5 - (i/2), 2);
		Synth(\sineperc_retrig, [\freq, ~degree53_to_freq.value(degree + (53*3)), \amp, 0.1, \trigBus, ~builtinSpectrumTriggers[i], \rel, rel, \out, ~functionCallBus]);
	};

	~drone = Synth(\windy, [\knob, 0.01, \amp, 0.0, \out, ~functionCallBus]);



	{ var counter = 0;
		loop {
			/*Pbind(\instrument, \osc,
			\dur, Pseq([1], 1),
			\freq, ~degree53_to_freq.value(~currentChord[0] + (53*2)),
			\buf, ~wt_buf[0],
			\atk, exprand(0.001, 0.02),
			\sus, 0,
			\rel, exprand(0.4, 0.6),
			\detune, rrand(0.01, 0.05) + (counter%2),
			\amp, 0.6 - (0.1 * (counter%2)),
			\pan, 0,
			\out, 0,
			\rout, ~rbus,
			\rsend, -16,
			).play;*/
			/*Pbind(\instrument, \osc,
			\freq, ~degree53_to_freq.value(~currentChord[0] + (53*2))*Pwrand([1, 2, 4], [0.8, 0.4, 0.2].normalizeSum, inf),
			\dur, Prand([
			Pseq([0.125], 8),
			// Pseq([0.25, 0.25, 0.25, 0.25]),
			// Pseq([0.25, 0.25, 0.25]),
			/*Pseq([Rest(0.125), 0.25, 0.25]),
			Pseq([0.125, 0.25, 0.125, 0.125]),
			Pseq([0.125, 0.125, 0.125, 0.125]*0.5),
			Pseq([0.5, 0.125, 0.25]),
			Pseq([Rest(0.5), 0.125, 0.25]),
			Ps eq([1]),*/
			]),
			\timingOffset, Pexprand(0.001, 0.01),
			\atk, exprand(0.001, 0.02),
			\sus, 0,
			\rel, 0.1,
			\c0, -3, // Pexprand(1, 5).neg,
			\c1, -4, // Pexprand(3, 10).neg,
			\detune, Pwhite(0.6, 0.65), // Pwhite(0.01, 0.05),
			\buf, ~wt_buf[0..3].choose,
			\amp, 0.2,
			\pan, 0,
			\out, 0,
			\rout, ~rbus,
			\rsend, -16,
			).play;*/
			counter = (counter + 1)%16;
			1.wait;
		}
	}.fork;

	OSCdef(\timeline_message, {
		|msg, time, addr, recvPort|
		// msg.postln;
		if(msg[1] == \functionCall) {
			var id = msg[2],
			parent = msg[3],
			scriptId = msg[4],
			parentScriptId = msg[5],
			withinScript = msg[6],
			notes = ~scriptChords[scriptId], // choose chord based on scriptId
			distance = parent - id, // todo: change to distance between scripts if it's a call outside
			idDigits = Set[], tempid = id;

			// if(~functionCallDensity < 99) {
			if(~doPlayFunctionCalls) {
				if(~scriptInfo[scriptId].scriptType == "built-in", {

					{ var index = (((scriptId/300.0)*~spectrumChord.size) % ~spectrumChord.size).floor,
						degree = ~spectrumChord[index];
						// ("built-in  index: " + index + "degree: " + degree + "scriptId" + scriptId).postln;
						~spectrumTriggers[index].set(1.0); // trigger the sineperc_retrig envelope
						/*{
						SinOsc.ar(~degree53_to_freq.value(degree + (53*3)), 0, 0.05)!2 * Env.perc(0.001, 2).kr(2);
						}.play;*/
					}.value();
					// };
				}, {
					{ var index = (((scriptId/300.0)*~builtinSpectrumChord.size) % ~builtinSpectrumChord.size).floor,
						degree = ~builtinSpectrumChord[index];
						// ("index: " + index + "degree: " + degree + "scriptId" + scriptId).postln;
						~builtinSpectrumTriggers[index].set(1.0); // trigger the sineperc_retrig envelope
						/*{
						SinOsc.ar(~degree53_to_freq.value(degree + (53*3)), 0, 0.05)!2 * Env.perc(0.001, 2).kr(2);
						}.play;*/
					}.value();
				});

				if(withinScript == 0) { var newChord, newScript = ~scriptInfo[scriptId];
					distance = ~scriptInfo[parentScriptId].interconnectedness[scriptId];

					if(newScript.scriptType == "built-in", {
						~currentRoot = ~currentRoot + 31; // subdominantisk riktning
						~currentDegrees = [0, 17, 31];
						// ("subdominantisk riktning dur").postln;
					}, {
						if(newScript.scriptType == "remote", {
							~currentRoot = ~currentRoot - 31; // dominantisk riktning
							~currentDegrees = [0, 17, 31, 44];
							// ("dominantisk riktning").postln;
						}, {
							if(distance > 0.1, {
								~currentRoot = ~currentRoot + 31; // subdominantisk riktning
								~currentDegrees = [0, 14, 31];
								// ("subdominantisk riktning moll").postln;
							}, {
								if(distance > 0.3, {
									~currentDegrees = ~currentDegrees.add(9);
								});
								if(distance > 0.6, {
									~currentDegrees = ~currentDegrees.add(49);
								});
							});
						});
					});
					~currentRoot = ~currentRoot%53;
					newChord = ~findClosestVoicing.(~currentChord, ~currentDegrees + ~currentRoot);
					~currentChord = newChord;
				}; // end within script if statement


				while({ tempid > 0 }, {
					idDigits = idDigits.add(tempid.asInt%10);
					tempid = (tempid.asInt/10).asInt;
				});

				if(~timeScale < 2.0) {
					~functionCallDensity = ~functionCallDensity + 0.5;
				};

				if(~timeScale < 1.5) {
					Synth(\tick2,[\freq, rrand(9000, 10000), \rq, 0.001, \amp, 0.03, \pan, [-1, 1].choose, \out, ~masterBus]);
				};
			};
		};

		if(msg[1] == \userEvent) {
			var type = msg[2].asString;
			msg.postln;

			if(~timeScale < 4.0) {
				if(type == "mousemove") {
					Synth(\friction_swoosh, [\amp, 0.5, \length, 0.5/~timeScale, \out, ~masterBus]);
				};

				if(type == "keydown") { var buf = ~clickSample;
					x = {
						var env = Env.linen(0.001, 0.5, 0.5).kr(2),
						sig = PlayBuf.ar(1, ~clickSample,
							BufRateScale.kr(~clickSample) * LFNoise1.kr(5).range(0.2, 0.3) * ~timeScale.linlin(0.05, 2.0, 0.2, 2.0),
							loop: 0)!2 * 0.8;
						sig = sig + PitchShift.ar(sig, 0.2, 0.25, 0.01, 0.00);
						sig = sig + (SinOsc.ar(60, 0, 0.01) * Env.perc(0.01, 0.2).kr(0));
						sig = sig * env;
						Out.ar(~masterBus, sig);
					}.play;
				};

				if(type == "click") { var buf = ~clickSample;
					x = {
						var sig = PlayBuf.ar(1, ~clickSample, BufRateScale.kr(~clickSample) * LFNoise1.kr(5).range(1.5, 3.0), loop: 0, doneAction: 2)!2 * 0.8;
						Out.ar(~masterBus, sig);
					}.play;
					Synth(\tick2,[\freq, 5000, \rq, 0.01, \amp, 0.03, \pan, 0, \out, ~masterBus]);
				};
			};

			/*if(type == "keyup") {
			var buf = ~clickSample, winenv;
			winenv = Env([0, 1, 0], [0.5, 0.5], [12, -12]);
			z = Buffer.sendCollection(s, winenv.discretize, 1);
			x = { var rate=Rand(0.05, 0.15), playhead, env, trig, sig;
			playhead = Line.ar(0.5, 1, (BufDur.ir(buf)/rate)*0.5, doneAction:2);
			env = EnvGen.kr(
			Env([0, 1, 1, 0], [0.001, BufDur.ir(buf)/rate, 1]),
			doneAction: Done.freeSelf);
			trig = Dust.kr(SinOsc.kr(1/5, 0.2).range(20, 40)) + Impulse.kr(0);
			sig = DelayN.ar(PlayBuf.ar(1, ~clickSample, BufRateScale.kr(~clickSample) * rate, startPos: BufFrames.ir(buf)*0.5, loop: 0)!2 * 0.5, 0.5, 0.5);
			sig = sig + GrainBuf.ar(2, trig, 0.8, buf, LFNoise1.kr(5).range(0.7, 1.0) , playhead, 2, 0, z);
			sig = sig + CombN.ar(sig, 0.2, 0.15, 0.5, 0.2);
			Out.ar(0, sig*env);
			}.play;
			};*/

			/*if(type == "load") { var octave = 0, currentTime, timeBetweenConsecutiveLoads = 1.0/~timeScale, freqs;
			~loadEventDensity = ~loadEventDensity + 0.1;
			~loadEventDensity.postln;
			if(~loadEventLastTime == 0) { ~loadEventLastTime = SystemClock.seconds; };
			currentTime = SystemClock.seconds;
			if(currentTime - ~loadEventLastTime < timeBetweenConsecutiveLoads, {
			~loadEventOctave = ~loadEventOctave + 1;
			}, {
			~loadEventOctave = 0;
			});
			~loadEventLastTime = currentTime;
			octave =  8 - (~loadEventOctave%6);
			freqs = ~currentChord.collect{|degree| ~degree53_to_freq.value(degree) };
			freqs = freqs.sort.reverse;
			if(~loadEventDensity < 0.8) {
			Pbind(\instrument, \osc,
			\freq, Pseq(freqs) * (2.pow(octave)),
			\dur, 0.125*0.5,
			\timingOffset, Pexprand(0.001, 0.01),
			\atk, exprand(0.05, 0.1),
			\sus, 0,
			\rel, exprand(0.8, 3.0),
			\c0, Pexprand(1, 2).neg,
			\c1, Pexprand(3, 10).neg,
			\detune, Pwhite(0.01, 0.05),
			\buf, ~wt_buf[1..3].choose,
			\amp, 0.2,
			\pan, 0,
			\out, 0,
			\rout, ~rbus,
			\rsend, -16,
			).play;
			};

			};
			*/
			if(type == "pageshow") {

			};

			if(type == "error") {

			};
		};
		if(msg[1] == \changeSpeed) {
			~timeScale = msg[2];
			~verbSynth.set(\wetness, 0.1 + ~timeScale.linlin(1.0, 20.0, 0.0, 0.9));
			~verbSynth.set(\feedback, 0.0 + ~timeScale.linlin(1.0, 60.0, 0.0, 0.85));
			msg.postln;
		};
		if(msg[1] == \startPlaying) {
			~master.set(\dustAmp, 1.0);
			msg.postln;
		};
		if(msg[1] == \timelineReset) {
			~generateChords.value(rrand(0, 53));
			~spectrumSynths.do{|syn, i| syn.set(\freq, ~degree53_to_freq.value(~spectrumChord[i] + (53*3))); };
			~builtinSpectrumSynths.do{|syn, i| syn.set(\freq, ~degree53_to_freq.value(~builtinSpectrumChord[i] + (53*3))); };
			msg.postln;
		};

	},
	'/timeline-message', n); // def style

	OSCdef(\script_info, {
		|msg, time, addr, recvPort|
		var script, index;
		msg.postln;

		script = (
			scriptId: msg[1],
			numFunctions: msg[2],
			scriptType: msg[3].asString,
			name: msg[4].asString,
			interconnectedness: (),
		);
		index = 5;
		while( {index+1 < msg.size}, {
			script.interconnectedness[msg[index]] = msg[index+1];
			index = index + 2;
		});
		~scriptInfo[script.scriptId] = script;
		script.postln;
	},
	'/script', n); // def style


	{
		loop { var normalized;
			// ~functionCallDensity = min(~functionCallDensity, 100);
			// ~functionCallDensity = ~functionCallDensity * 0.996;
			~functionCallDensity = ~functionCallDensity * (1.0 - (~timeScale * 0.02));
			// ~functionCallDensity = max(~functionCallDensity, 0);
			normalized = min((~functionCallDensity/200.0), 1.0);
			// ~speedSynths.do{|syn| syn.set(\maxDist, normalized, \maxRoughness, normalized); };
			~drone.set(\amp, normalized*1.5);
			~drone.set(\cutoff, normalized.pow(3).linexp(0, 1.0, 1000, 100));
			0.01.wait;
		}
	}.fork;
	{
		loop { var normalized;
			~loadEventDensity = min(~loadEventDensity, 1.0);
			~loadEventDensity = ~loadEventDensity * 0.98;
			~loadEventDensity = max(~loadEventDensity, 0);
			0.01.wait;
		}
	}.fork;
});
)



( var port;
~maxCap = 0;
~slowThresh = 390;
~fastThresh = 700;
~ofAddr = NetAddr("127.0.0.1", 12000);
~sendCapToOF = { | capValue |

};

~sendTimeScaleToOF = { | timeScale |
	~timeScale = timeScale;
	~ofAddr.sendMsg(\timeScale, timeScale);
};

{
	SerialPort.closeAll;
	1.0.wait;
	port = SerialPort.devices[0];
	("Connecting to " + port).postln;
	p = SerialPort(
		port,
		baudrate: 115200,
		crtscts: true);

	~readLoop = fork {
		var byte, str, capLPF = 0, timeScale = 0;
		loop{|i|
			if(p.read==10, { var amp, knob, capValue, dustAmp = 0.0, functionCallAmp = 0.0;
				str = "";
				while({byte = p.read; byte !=13 }, {
					str= str++byte.asAscii;
				});
				capValue = str.asInt;
				if(capValue >  ~maxCap) {
					~maxCap = capValue;
				};
				~maxCap = ~maxCap * 0.99999;

				str.postln;
				amp = capValue.linlin(280, 500, 0.0, 1.0);
				// ("amp: " + amp.asString).postln;

				capLPF = (capValue*0.001) + (capLPF*0.999);

				if(capValue < ~slowThresh, {
					timeScale = 0;
				});
				if(capValue >= ~slowThresh, {
					timeScale = capValue.linlin(~slowThresh, ~slowThresh*1.3, 1.0, 0.5);
					~doPlayFunctionCalls = false;
				});
				if(capValue >= ~fastThresh, {
					timeScale = capValue.linlin(~fastThresh, ~fastThresh*1.3, 0.5, 0.2);
					~doPlayFunctionCalls = true;
					dustAmp = capValue.linexp(~fastThresh, ~fastThresh*1.2, 0.01, 1.0);
					functionCallAmp = 1.0;
				});
				~sendTimeScaleToOF.(timeScale);
				knob = capLPF.linlin(200, ~maxCap * 0.8, 1.0, 0.0);
				~master.set(\amp, amp);
				~master.set(\dustAmp, dustAmp);
				~master.set(\functionCallAmp, functionCallAmp);
			});
		};

	};
}.fork;
)









~filteredNoiseSynths.do{|syn| syn.release; }; // release all old chords
(
20.do {
	("size: " + ~filteredNoiseSynths.size).postln;
	~filteredNoiseSynths.do{|syn| syn.release; }; // release all old chords
	~filteredNoiseSynths = 4.collect {var release = 10, freq = rrand(3000, 4000);
		//("filtered noise with a release of " + release).postln;
		Synth(\filtered_noise, [\freq, freq*0.25, \amp, 1.0, \atk, 0.1, \rel, release, \pan, rrand(-1, 1)]);
	};
}
)
~scriptInfo[0].scriptType == \built-in;

Synth(\tick2,[\freq, rrand(200, 300), \rq, 0.01, \amp, 0.1, \pan, [-1, 1].choose]);

(
var buf = ~clickSample, winenv;
winenv = Env([0, 1, 0], [0.5, 0.5], [12, -12]);
z = Buffer.sendCollection(s, winenv.discretize, 1);
x = { arg rate=0.1; var playhead, env, trig, sig;
	playhead = Line.ar(0.5, 1, (BufDur.ir(buf)/rate)*0.5, doneAction:2);
	env = EnvGen.kr(
		Env([0, 1, 1, 0], [0.001, BufDur.ir(buf)/rate, 1]),
		doneAction: Done.freeSelf);
	trig = Dust.kr(SinOsc.kr(1/5, 0.2).range(20, 40)) + Impulse.kr(0);
	sig = DelayN.ar(PlayBuf.ar(1, ~clickSample, BufRateScale.kr(~clickSample) * rate, startPos: BufFrames.ir(buf)*0.5, loop: 0)!2 * 0.5, 0.5, 0.5);
	sig = sig + GrainBuf.ar(2, trig, 0.8, buf, 1.0, playhead, 2, 0, z);
	sig = sig + CombN.ar(sig, 0.2, 0.15, 0.5, 0.2);
	Out.ar(0, sig*env);
}.play;
)


[1,3,2].sort.reverse
~spectrumTriggers[15].set(0.2)
50.linlin(1.0, 60.0, 0.0, 0.85)