
// Constants for the inverse PQ (ST.2084) transfer function
const float m1 = 2610.0 / 16384.0;   // 0.1593017578125
const float m2 = 2523.0 / 4096.0;    // 0.6162109375
const float c1 = 3424.0 / 4096.0;    // 0.8359375
const float c2 = 2413.0 / 4096.0;    // 0.58935546875
const float c3 = 2392.0 / 4096.0;    // 0.5849609375


float inversePQ_01texture(float N) {
    // N is the non‑linear value in [0,1]
    // Linear output L is normalized to [0,1] (1 → 10000 cd/m²)
    if (N <= 0.0) return 0.0;                 // avoid undefined pow(0, exponent)
    float Np = pow(N, 1.0 / m2);
    float num = max(Np - c1, 0.0);
    float den = c2 - c3 * Np;
    float Lp = num / den;
    float L = pow(Lp, 1.0 / m1);
    return L;
}

