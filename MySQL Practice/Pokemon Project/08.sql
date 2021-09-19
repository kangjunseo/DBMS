SELECT COUNT(*)
FROM Pokemon, CatchedPokemon
WHERE Pokemon.id = pid
GROUP BY type
ORDER BY type
;
