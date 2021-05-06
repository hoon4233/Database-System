SELECT DISTINCT Pokemon.name
FROM Pokemon, CatchedPokemon
WHERE Pokemon.id = pid
and pid IN
( SELECT pid FROM Trainer, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id and Trainer.hometown = 'Sangnok City' )
and pid IN
( SELECT pid FROM Trainer, CatchedPokemon WHERE Trainer.id = CatchedPokemon.owner_id and Trainer.hometown = 'Brown City' )
ORDER BY Pokemon.name