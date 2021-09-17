SELECT name
FROM Trainer
WHERE name NOT IN(
	SELECT name
	FROM Trainer, Gym
	WHERE id = leader_id
);

