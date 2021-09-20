SELECT Trainer.name
FROM Trainer, CatchedPokemon, Pokemon
WHERE Trainer.id = owner_id AND pid = Pokemon.id
AND Pokemon.name = 'Pikachu'
ORDER BY Trainer.name
;
