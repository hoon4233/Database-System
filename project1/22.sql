SELECT type, COUNT(id)
FROM Pokemon
GROUP BY type
ORDER BY COUNT(id), type