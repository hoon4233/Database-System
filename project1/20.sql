SELECT Trainer.name, COUNT(CatchedPokemon.id)
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id
and Trainer.hometown = 'Sangnok City'
GROUP BY Trainer.name
ORDER BY COUNT(CatchedPokemon.id)