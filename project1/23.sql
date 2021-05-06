SELECT DISTINCT Trainer.name
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id
and CatchedPokemon.level <= 10
ORDER BY Trainer.name