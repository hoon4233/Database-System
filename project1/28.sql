SELECT Trainer.name, AVG(CatchedPokemon.level)
FROM Trainer, CatchedPokemon
WHERE Trainer.id = CatchedPokemon.owner_id
and CatchedPokemon.pid 
IN ( (SELECT id FROM Pokemon WHERE Pokemon.type = 'Normal' or Pokemon.type ='Electric') )
GROUP BY Trainer.id
ORDER BY AVG(CatchedPokemon.level)