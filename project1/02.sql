SELECT p1.name
FROM Pokemon p1
WHERE p1.type IN (
	SELECT p2.type
    FROM Pokemon p2
	GROUP BY p2.type
	HAVING count(p2.type) >= (SELECT COUNT(p3.id) FROM Pokemon p3 GROUP BY p3.type ORDER BY COUNT(p3.id) DESC LIMIT 1,1)
)
ORDER BY p1.name