

R = 1;
L = 0.01;
vol = 100;
freq = 1:1:50;
z = (R + i*(2*pi.*freq*L));
cur = vol ./ z ;

a=angle(cur)*180.0/pi;

S = abs(vol) .* abs(cur);

P = abs(cur) .^ 2 .* R;

subplot( 3, 3, 1 );

plot( freq, abs(P) )
grid;
xlabel("freq");
ylabel("P");

subplot( 3, 3, 2 );

plot( freq, abs(S) )
grid;
xlabel("freq");
ylabel("S");

subplot( 3, 3, 3 );
plot( freq, abs(cur) );
grid;
xlabel("freq");
ylabel("cur");


subplot( 3, 3, 4 );

plot( freq, angle(cur)*180/pi );
grid;
xlabel("freq");
ylabel("angle");


