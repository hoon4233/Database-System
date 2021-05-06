SELECT name
FROM Pokemon, Evolution
WHERE Pokemon.type = 'Grass' and Pokemon.id = Evolution.before_id