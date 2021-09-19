SELECT nickname
FROM Gym, Trainer, CatchedPokemon, Pokemon
WHERE leader_id = Trainer.id 
AND Trainer.id = owner_id 
AND Pokemon.id = pid
AND city = 'Sangnok City'
AND type = 'Water'
ORDER BY nickname
;
