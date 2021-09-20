SELECT Pokemon.name
FROM Gym, Trainer, CatchedPokemon, Pokemon
WHERE leader_id = Trainer.id AND city = 'Rainbow City'
AND owner_id = Trainer.id AND pid = Pokemon.id
ORDER BY Pokemon.name
;
