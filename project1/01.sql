SELECT name
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id
GROUP BY Trainer.name
HAVING COUNT(CatchedPokemon.pid) >= 3
ORDER BY COUNT(CatchedPokemon.pid)