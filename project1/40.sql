SELECT t1.hometown, c1.nickname
FROM Trainer t1, CatchedPokemon c1
WHERE t1.id = c1.owner_id
and c1.level IN (
    SELECT MAX(level) 
    FROM Trainer t2, CatchedPokemon c2
    WHERE t2.id = c2.owner_id
    GROUP BY t2.hometown
    )
GROUP BY t1.hometown
ORDER BY t1.hometown