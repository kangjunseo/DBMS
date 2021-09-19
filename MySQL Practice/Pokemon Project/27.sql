SELECT nickname
FROM Trainer, CatchedPokemon
WHERE owner_id = Trainer.id
AND level>=40 AND owner_id>=5
ORDER BY nickname
;
