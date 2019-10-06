const sqlite3 = require('sqlite3');
var http = require('http')
var express = require('express')
var app = express();
const multer = require('multer');
var fs = require('fs');
var uniqid = require('uniqid');
var dbname = 'test.db';

app.use(express['static'](__dirname));

app.get('/current', function(req, res) {
    let db = new sqlite3.Database(dbname, (err) => {
	if (err) {
	    console.error(err.message);
	}
    });
    let request = 'SELECT IMAGE_ID id FROM CURRENT_IMAGE';
    try {
	db.get(request, (err, row) => {
	    if (err) {
		res.status(500).send('INTERNAL DB ERROR->' + err);
	    }
	    res.status(200).send({'selected_image': row ? row.id : -1});
	});	
    } catch (err) {
	res.status(500).send('OOPS' + err);
    }
    db.close()
});

app.get('/library', function(req, res) {
    let db = new sqlite3.Database(dbname, (err) => {
	if (err) {
	    console.error(err.message);
	}
    });
    let request = 'SELECT ID id, FILENAME filename FROM LIBRARY';
    try {
	db.all(request, [], (err, rows) => {
	    if (err) {
		res.status(500).send('INTERNAL DB ERROR->' + err);
	    }
	    var ids = []
	    rows.forEach((row) => {
		ids.push(row.id);
	    });
	    res.status(200).send({'library': ids});
	});
    } catch (err) {
	res.status(500).send('OOPS' + err);
    }
    db.close()
});

app.put('/current/:id', function(req, res) {
    let request = 'SELECT ID id FROM LIBRARY WHERE ID = ?';
    let db = new sqlite3.Database(dbname, (err) => {
	if (err) {
	    console.error(err.message);
	}
    });
    try {
	var found = false;
	db.get(request, [req.params.id], (err, row) => {
	    if (err) {
		res.status(500).send('INTERNAL DB ERROR->' + err);
	    }
	    found = row ? true : false;
	    if (found == false) {
		res.status(400).send('image not in lib');
	    } else {
		var success = false;
		let replace_request = 'UPDATE CURRENT_IMAGE SET IMAGE_ID = ? WHERE ID = 0';
		
		db.run(replace_request, [req.params.id], function(err){
		    if (err) {
			res.status(500).send('INTERNAL DB ERROR->' + err);
		    }
		
		    success = err ? false : true;
		    if (success) {
			res.status(200).send({'selected_image': req.params.id});
		    } else {
			res.status(500).send('error while updating db' + err);
		    }
		});
	    }
	});
    } catch (err) {
	res.status(500).send('OOPS' + err);
    }
    db.close();
})

// SET STORAGE
var storage = multer.diskStorage({
    destination: function (req, file, cb) {
	cb(null, 'tmp')
    },
    filename: function (req, file, cb) {
	cb(null, file.fieldname + '-' + Date.now())
    }
});

var upload = multer({ storage: storage });

app.post('/library', upload.single('picture'),  function(req, res, next){
    const file = req.file
    if (!file) {
	const error = new Error('Please upload a file');
	error.httpStatusCode = 400;
	return next(error);
    }

    var path = "tmp/" + file.filename;
    var image_ok = true;

    if (image_ok) {
	let request = 'INSERT INTO LIBRARY VALUES(null, ?, datetime())';
	let db = new sqlite3.Database(dbname, (err) => {
	    if (err) {
		console.error(err.message);
	    }
	});
	try {
	    var filename = uniqid() + ".png";
	    var target_path = "library/" + filename;
	    
	    db.run(request, [filename], function(err) {
		if (err) {
		    res.status(500).send('INTERNAL DB ERROR->' + err);
		    return;
		}

		fs.renameSync(path, target_path);
		let sql = 'SELECT ID id, FILENAME file FROM LIBRARY where FILENAME = ?';
		db.get(sql, [filename], (err, row) => {
		    if (err) {
			res.status(500).send('INTERNAL DB ERROR->' + err);
		    }
		    if (!row) {
			res.status(500).send('NO ROW FOUND');
			return;
		    }

		    res.status(200).send({'new_item': row.id});
		});
	    });
	} catch (err) {
	    res.status(500).send('OOPS' + err);
	}
	db.close();
    } else {
	//deletion
	var path = "tmp/" + file.filename;
	fs.unlink(path, (err) => {
	    if (err) {
		console.error(err)
		return;
	    }
	});
    }
});

app.delete('/library/:id', function(req, res) {
    let db = new sqlite3.Database(dbname, (err) => {
	if (err) {
	    console.error(err.message);
	}
    });
    let request = 'SELECT ID id, FILENAME file FROM LIBRARY WHERE ID = ?';
	try {
		var found = false;
		db.get(request, [req.params.id], (err, row) => {
			if (err) {
			res.status(500).send('INTERNAL DB ERROR->' + err);
			}
			found = row ? true : false;
			if (found == false) {
				res.status(400).send('image not in lib');
			} else {
				const path = `${__dirname}/library/` + row.file;
				
				let request = 'DELETE FROM LIBRARY WHERE ID = ?';

				db.run(request, [req.params.id], (err) => {
					if (err) {
						res.status(500).send('INTERNAL DB ERROR->' + err);
					}

					res.status(200).send({ 'deleted_item': req.params.id });
				});

				fs.unlink(path, (err) => {
					if (err) {
					console.error(err)
					return;
					}
				});
			}
		});
		} catch (err) {
			res.status(500).send('OOPS' + err);
		}
		db.close();
});

app.get('/library/:id', function(req, res) {
    let db = new sqlite3.Database(dbname, (err) => {
	if (err) {
	    console.error(err.message);
	}
    });
    let request = 'SELECT ID id, FILENAME file FROM LIBRARY WHERE ID = ?';
	try {
		var found = false;
		db.get(request, [req.params.id], (err, row) => {
			if (err) {
			res.status(500).send('INTERNAL DB ERROR->' + err);
			}
			found = row ? true : false;
			if (found == false) {
				res.status(400).send('image not in lib');
			} else {
				const file = `${__dirname}/library/` + row.file;
				res.download(file);
			}
		});
		} catch (err) {
			res.status(500).send('OOPS' + err);
		}
		db.close();
});

app.get('/download', function(req, res){
    const file = `${__dirname}/myapi.js`;
    res.download(file);
});

app.get('*', function(req, res) {
    res.status(404).send('Unrecognised API call'); 
});

app.use(function(err, req, res, next) {
    if (req.xhr) {
	res.status(500).send('OOPS, seomthing went wrong');
    } else {
	next(err);
    }
});

app.listen(3000);
