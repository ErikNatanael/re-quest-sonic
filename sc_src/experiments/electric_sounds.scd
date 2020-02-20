// electric sounds

( // loads of dust
{
	var sig, freqs, trigs, changeTrig = Changed.kr(MouseY.kr() > 0.5);
	freqs = 10000 * [0.7, 0.6, 0.8, 0.9, 0.95, 1.0];
	trigs = {Dust.ar(TRand.ar(10, 40, changeTrig))}!6;
	trigs = Decay.ar(trigs, 0.0001);
	sig = SinOsc.ar(freqs) * trigs * 0.1;
	sig = Splay.ar(sig);
	Out.ar(0, sig);
}.play;
)

( // loads of dust, adjustable pitchness
{
	var sig, freqs, trigs;
	freqs = MouseY.kr(500, 10000, 1) * [0.7, 0.6, 0.8, 0.9, 0.95, 1.0];
	trigs = {Dust.ar(Rand(10, 40))}!6;
	trigs = Decay.ar(trigs, MouseX.kr(0.00001, 0.05, 1));
	sig = SinOsc.ar(freqs) * trigs * 0.1;
	sig = Splay.ar(sig);
	Out.ar(0, sig);
}.play;
)

( // single speck of dust
SynthDef(\speck, {
	var sig, freq, trigs;
	Line.kr(dur: 0.05, doneAction: 2);
	freq = 10000 * Rand(0.6, 1.0);
	trigs = Impulse.ar(0);
	trigs = Decay.ar(trigs, 0.0001);
	sig = SinOsc.ar(freq) * trigs * 0.1;
	sig = Pan2.ar(sig, Rand(-1, 1));
	Out.ar(0, sig);
}).add;
)
(
{
	loop {
		Synth(\speck);
		rrand(0.0001, 0.05).wait;
	}
}.fork
)
