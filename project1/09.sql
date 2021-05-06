SELECT name, AVG(level)
FROM Trainer, Gym, CatchedPokemon
WHERE Trainer.id = Gym.leader_id and Trainer.id = CatchedPokemon.owner_id
GROUP BY Trainer.name
ORDER BY Trainer.name