SELECT p1.type
FROM Pokemon p1, Evolution e1
WHERE p1.id = e1.before_id
GROUP BY p1.type
HAVING COUNT(p1.id) >= 3
ORDER BY p1.type DESC