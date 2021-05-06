SELECT DISTINCT t1.name
FROM Trainer t1, CatchedPokemon c1
WHERE t1.id = c1.owner_id
and c1.pid IN (
    SELECT p1.id
    FROM Pokemon p1 JOIN Evolution e1 ON p1.id = e1.after_id LEFT OUTER JOIN Evolution e2 ON e1.after_id = e2.before_id
    WHERE e2.before_id is NULL
)
ORDER BY t1.name