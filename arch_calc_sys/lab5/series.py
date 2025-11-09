import math

pi = math.pi
q = pi ** 2
sum_total = 0.0

for n in range(1, 51):
    denominator = 2 * n * n - 1
    numerator = 2 * n + 1
    fraction = numerator / denominator
    
    if fraction < 1.0:
        log_part = math.log10(1.0 - fraction)
        pow_part = 1.0 / (q ** n)
        sum_total += log_part + pow_part
    
    print(f"n = {n}, S = {sum_total}")