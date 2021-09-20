SELECT DISTINCT Trainer.name
FROM Trainer, CatchedPokemon, Pokemon
WHERE Trainer.id = owner_id AND pid = Pokemon.id
AND type = 'Psychic'
ORDER BY Trainer.name
;
