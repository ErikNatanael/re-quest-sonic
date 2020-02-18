(
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
)

(
n = NetAddr("127.0.0.1", 57120); // local machine
// why does this not work, but the OSCFunc below does?
OSCdef(\time, {|msg, time, addr, recvPort| msg.postln}, '/capTime', n); // def style
)

(
~capTime = [];
o = OSCFunc({
	arg msg, time, addr, recvPort;
	~capTime = msg[1..];
	// ~capTime.postln;
}, '/capTime', nil, 57120);
)
(
~capVoltage = [];
o = OSCFunc({
	arg msg, time, addr, recvPort;
	~capVoltage = msg[1..];
	// ~capVoltage.postln;
	~parseCapacitiveData.();
}, '/capVoltage', nil, 57120);
)
(
~synth = Synth(\windy);
~maxVoltageIndex = 0;
~totalVoltage = 0;
~parseCapacitiveData = {
	var maxVoltage = 0, amp = 0, freq;
	~totalVoltage = 0;
	~capVoltage.do {|voltage, i|
		if(voltage > maxVoltage, {
			~maxVoltageIndex = i;
			maxVoltage = voltage;
		});
		~totalVoltage = ~totalVoltage + voltage;
	};
	amp = max(40000 - ~totalVoltage, 0.0)/20000.0;
	~synth.set(\amp, amp);
	freq = ~maxVoltageIndex.linlin(0, 159, 0.0, 1.0);
	~synth.set(\knob, freq);
	("max voltage: " + ~maxVoltageIndex.asString).postln;
	("total voltage: " + ~totalVoltage.asString).postln;
}
)