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
SELECT DISTINCT a.airplane_code, a.model
FROM Airplanes a
WHERE NOT EXISTS (
    SELECT 1
    FROM Routes r
    JOIN Flights f ON r.route_no = f.route_no
    WHERE r.airplane_code = a.airplane_code 
    AND f.status != 'Arrived'
)
AND EXISTS (
    SELECT 1
    FROM Routes r
    JOIN Flights f ON r.route_no = f.route_no
    WHERE r.airplane_code = a.airplane_code
);