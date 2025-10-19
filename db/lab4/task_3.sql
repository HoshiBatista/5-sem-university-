-- 1. Найдите пассажиров, у которых есть посадочные талоны на рейс с flight_id = 12345
SELECT DISTINCT t.passenger_name, bp.seat_no
FROM Boarding_passes bp
JOIN Tickets t ON bp.ticket_no = t.ticket_no
WHERE bp.flight_id = 12345;

-- 2. Найти все посадочные талоны, выданные на рейс с flight_id = 12345
SELECT ticket_no, seat_no, boarding_no
FROM Boarding_passes
WHERE flight_id = 12345;

-- 3. Найдите модели самолётов, у которых все рейсы имеют статус 'Arrived'
SELECT 
    a.airplane_code,
    a.model
FROM Airplanes a
JOIN Routes r ON a.airplane_code = r.airplane_code
JOIN Flights f ON r.route_no = f.route_no
GROUP BY a.airplane_code, a.model
HAVING COUNT(f.flight_id) = SUM(CASE WHEN f.status = 'Arrived' THEN 1 ELSE 0 END)
   AND COUNT(f.flight_id) > 0;
