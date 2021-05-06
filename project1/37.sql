SELECT t1.name, SUM(c1.level)
FROM Trainer t1, CatchedPokemon c1
WHERE t1.id = c1.owner_id
GROUP BY t1.id
HAVING SUM(c1.level) = (
    SELECT SUM(c2.level)
    FROM Trainer t2, CatchedPokemon c2 
    WHERE t2.id = c2.owner_id 
    GROUP BY t2.id 
    ORDER BY SUM(c2.level) DESC
    LIMIT 1
)
ORDER BY t1.name