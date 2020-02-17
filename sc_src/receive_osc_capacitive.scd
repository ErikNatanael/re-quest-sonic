(
n = NetAddr("127.0.0.1", 57120); // local machine
// why does this not work, but the OSCFunc below does?
OSCdef(\time, {|msg, time, addr, recvPort| msg.postln}, '/capTime', n); // def style
)

n.sendMsg("/capTime", "Hello App 1");

(
o = OSCFunc({
	arg msg, time, addr, recvPort;
	msg.postln;
}, '/capTime', nil, 57120);
)
(
o = OSCFunc({
	arg msg, time, addr, recvPort;
	msg.postln;
}, '/capVoltage', nil, 57120);
)