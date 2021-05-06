SELECT City.name, AVG(level)
FROM City, Trainer, CatchedPokemon
WHERE City.name = Trainer.hometown and Trainer.id = CatchedPokemon.owner_id
GROUP BY City.name
ORDER BY AVG(level)