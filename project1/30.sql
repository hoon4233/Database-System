SELECT p1.id, p1.name, p2.name, p3.name
FROM Pokemon p1, Pokemon p2, Pokemon p3
WHERE p1.id <> p2.id and p2.id <> p3.id and
(p1.id, p2.id, p3.id)
IN (SELECT e1.before_id, e1.after_id, e2.after_id FROM Evolution e1, Evolution e2 WHERE e1.after_id = e2.before_id)
ORDER BY p1.id

