SELECT Trainer.name, MAX(CatchedPokemon.level)
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id
GROUP BY Trainer.id
HAVING COUNT(CatchedPokemon.id) >= 4
ORDER BY Trainer.name