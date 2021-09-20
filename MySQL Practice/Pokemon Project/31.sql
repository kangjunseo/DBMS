SELECT Sname
FROM(
  SELECT Pokemon.name AS Sname
  FROM CatchedPokemon, Trainer, Pokemon
  WHERE Trainer.id = owner_id AND pid = Pokemon.id AND
  hometown = 'Sangnok City' ) AS SC,(
  SELECT Pokemon.name AS Bname
  FROM CatchedPokemon, Trainer, Pokemon
  WHERE Trainer.id = owner_id AND pid = Pokemon.id AND
  hometown = 'Blue City' ) AS BC
WHERE Sname = Bname
ORDER BY Sname
;
