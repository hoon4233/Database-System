SELECT AVG(level)
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id and Trainer.name = 'Red'