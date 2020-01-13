

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
	SynthDef(\windy, { |outBus = 0, knob = 0.5, decay = 10, gate = 1, amp = 1|
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

		Out.ar(outBus, [outs, DelayN.ar(outs, 0.01, 0.01)]);
	}).add;
	SynthDef(\swoosh, {
		var sig, env, freq, clap, popEnv, centerFreq = Rand(100, 200), length = Rand(0.2, 0.7);
		env = EnvGen.ar(
			Env(
				levels: [0, 1, 0],
				times: [length, 0.3],
				curve: [0, -2]
			),
			doneAction:2
		);
		popEnv = EnvGen.ar(Env([3261, 410, Rand(30, 80)], [0.002, 0.01], curve: \exp));


		freq = [
			EnvGen.ar(Env([centerFreq*Rand(0.5, 1.2), centerFreq], [length], [Rand(4, 7)])),
			EnvGen.ar(Env([centerFreq*Rand(0.2, 0.4), centerFreq], [length], [Rand(2, 8)]))];
		sig = Resonz.ar(WhiteNoise.ar(), freq, Rand(0.05, 0.2))*4;
		sig = Mix(sig);
		sig = Pan2.ar(sig, Line.kr(Rand(-1, 1), Rand(-1, 1), length+1));
		sig = sig * env * 0.5 * \amp.kr(0.5);
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
	SynthDef(\sineperc_retrig, { var freq = \freq.kr(100), sig, env;
		env = EnvGen.kr(
			Env.perc(\atk.kr(0.01), \rel.kr(1), 1.0, [\c0.kr(4), \c1.kr(-4)]),
			gate: InTrig.kr(\trigBus.kr(10))
			// gate: \gate.kr(0)
		);
		freq = XLine.ar(Rand(4000, 6000), freq, 0.006);
		sig = SinOsc.ar(freq) * 0.5 * \amp.kr(1);
		sig = sig * env;
		sig = Pan2.ar(sig, \pan.kr(0));
		Out.ar(0, sig);
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
		var snd, wet;
		snd = In.ar(\out.kr(0), 2);
		wet = snd.sum + PitchShift.ar(snd, 0.2, 2);
		wet = DelayN.ar(wet, 0.02, 0.02);
		wet = JPverb.ar(wet, 4.0, 0.0, 1.8);
		wet = HPF.ar(wet, 100) * -3.dbamp;
		wet = LPF.ar(wet, 7000);
		snd = snd + (wet * \wetness.kr(0.1));
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
	~clickSample = Buffer.readChannel(s, "../samples/442649__asetkeyzet__keyboard-click.wav".resolveRelative, channels: [0]);
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
				// check distance
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
	Synth(\verb, [\wetness, 0.3]);


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

	{var lastNote;
		~spectrumChord = 32.collect{|i| var newNote;
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
			newNote
		};
	}.value();

	~speedSynths = 4.collect{|i| var offset = [0, 17, 31, 47][i];
		Synth(\noisepad, [\freq, ~degree53_to_freq.(~currentRoot + offset + (53*5))]);
	};

	~spectrumTriggers = ~spectrumChord.collect{|degree, i| Bus.control(s, 1);};
	~spectrumSynths = ~spectrumChord.collect{|degree, i|
		Synth(\sineperc_retrig, [\freq, ~degree53_to_freq.value(degree + (53*3)), \amp, 0.1, \trigBus, ~spectrumTriggers[i]]);
	};

	~drone = Synth(\windy, [\knob, 0.01, \amp, 0.0]);



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
				{ var index = ((scriptId/300.0)*~spectrumChord.size).floor,
					degree = ~spectrumChord[index];
					("index: " + index + "degree: " + degree + "scriptId" + scriptId).postln;
				~spectrumTriggers[index].set(1.0); // trigger the sineperc_retrig envelope
					/*{
					SinOsc.ar(~degree53_to_freq.value(degree + (53*3)), 0, 0.05)!2 * Env.perc(0.001, 2).kr(2);
					}.play;*/
				}.value();
		// };

			if(withinScript == 0) { var newChord, newScript = ~scriptInfo[scriptId];

				// Synth(\tick2,[\freq, rrand(5000, 10000), \rq, 0.1, \amp, 1.0, \pan, [-1, 1].choose]);
				// switch to new root note
				distance = ~scriptInfo[parentScriptId].interconnectedness[scriptId];
				/*~drone.set(\knob, distance);*/
				/*if(distance > 0.9, {
				~currentRoot = ~currentRoot + 31; // subdominantisk riktning
				~currentDegrees = [0, 17, 31];
				("subdominantisk riktning dur").postln;
				}, {
				if(distance > 0.5, {
				~currentRoot = ~currentRoot - 31; // dominantisk riktning
				~currentDegrees = [0, 17, 31, 44];
				("dominantisk riktning").postln;
				}, {
				if(distance > 0.1, {
				~currentRoot = ~currentRoot + 31; // subdominantisk riktning
				~currentDegrees = [0, 14, 31];
				("subdominantisk riktning moll").postln;
				}, {
				if(distance > 0.2, {
				~currentDegrees = ~currentDegrees.add(9);
				});
				if(distance > 0.3, {
				~currentDegrees = ~currentDegrees.add(49);
				});
				});
				});

				});*/
				// change root according to script type
				// ("scriptType: " + newScript.scriptType).postln;
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
				// ("newChord: " + newChord).postln;
				// change background chord
				~speedSynths.do{|syn| syn.set(\amp, 0.0); };
				/*newChord.do{|note, i|
					var syn = ~speedSynths[i];
					syn.set(\freq, ~degree53_to_freq.(note + (53*(4+(i%3)))));
					syn.set(\amp, 1.0);
				};*/
				~currentChord = newChord;
			}; // end within script if statement


			while({ tempid > 0 }, {
				idDigits = idDigits.add(tempid.asInt%10);
				tempid = (tempid.asInt/10).asInt;
			});

			~functionCallDensity = ~functionCallDensity + 0.5;

			// trigger a fragment change
			~speedSynths.do{|syn| syn.set(\fragmentTrigger, rrand(0.0, 10.0)); };

			Synth(\tick2,[\freq, rrand(9000, 10000), \rq, 0.001, \amp, 0.1, \pan, [-1, 1].choose]);

			// kind of just gives you an impression of density
			// Synth(\tick,[\freq, rrand(5000, 6000)*~speed.linlin(0.0, 2.0, 0.1, 2.0), \rq, ~functionCallDensity.linexp(0, 1, 0.001, 1.0), \amp, min(~speed.pow(2), 1.0), \pan, [-1, 1].choose]);
			// one note per scriptId as of right now
			// Synth(\sineperc, [\freq, ~degree53_to_freq.(~currentRoot + [0, 17, 31, 9].choose + (53*6)), \amp, 0.05, \atk, 0.1, \rel, rrand(0.2, 1.5) + (1/~functionCallDensity)*2, \pan, rrand(-1, 1.0)]);
		};

		if(msg[1] == \userEvent) {
			var type = msg[2].asString;
			msg.postln;

			if(type == "mousemove") {
				Synth(\swoosh, [\amp, 0.5]);
			};

			if(type == "keydown") { var buf = ~clickSample;
				x = { PlayBuf.ar(1, ~clickSample, BufRateScale.kr(~clickSample) * LFNoise1.kr(5).range(0.2, 0.3), loop: 0)!2 * 0.5 }.play;
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
			~functionCallDensity = ~functionCallDensity * 0.995;
			// ~functionCallDensity = max(~functionCallDensity, 0);
			normalized = min((~functionCallDensity/200.0), 1.0);
			~speedSynths.do{|syn| syn.set(\maxDist, normalized, \maxRoughness, normalized); };
			~drone.set(\amp, normalized);
			~drone.set(\cutoff, normalized.pow(3).linexp(0, 1.0, 1000, 100));
			0.01.wait;
		}
	}.fork;
	{
		loop { var normalized;
			~loadEventDensity = min(~loadEventDensity, 1.0);
			~loadEventDensity = ~loadEventDensity * 0.99;
			~loadEventDensity = max(~loadEventDensity, 0);
			0.01.wait;
		}
	}.fork;
});
)

a = Synth(\filtered_noise, [\freq, 4000*0.25, \amp, 1.0, \atk, 0.1, \rel, 10, \pan, rrand(-1, 1)]);
a.release
a.set(\gate, 0);

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