
#include <stdlib.h>

#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "vec2.h"
#include "math.h"
#include <stdio.h>

// Οι ολοκληρωμένες πληροφορίες της κατάστασης του παιχνιδιού.
// Ο τύπος State είναι pointer σε αυτό το struct, αλλά το ίδιο το struct
// δεν είναι ορατό στον χρήστη.

struct state {
	Vector objects;			// περιέχει στοιχεία Object (αστεροειδείς, σφαίρες)
	struct state_info info;	// Γενικές πληροφορίες για την κατάσταση του παιχνιδιού
	int next_bullet;		// Αριθμός frames μέχρι να επιτραπεί ξανά σφαίρα
	float speed_factor;		// Πολλαπλασιαστής ταχύτητς (1 = κανονική ταχύτητα, 2 = διπλάσια, κλπ)
};


// Δημιουργεί και επιστρέφει ένα αντικείμενο

static Object create_object(ObjectType type, Vector2 position, Vector2 speed, Vector2 orientation, double size) {
	Object obj = malloc(sizeof(*obj));
	obj->type = type;
	obj->position = position;
	obj->speed = speed;
	obj->orientation = orientation;
	obj->size = size;
	return obj;
}

// Επιστρέφει έναν τυχαίο πραγματικό αριθμό στο διάστημα [min,max]

static double randf(double min, double max) {
	return min + (double)rand() / RAND_MAX * (max - min);
}

// Προσθέτει num αστεροειδείς στην πίστα (η οποία μπορεί να περιέχει ήδη αντικείμενα).
//
// ΠΡΟΣΟΧΗ: όλα τα αντικείμενα έχουν συντεταγμένες x,y σε ένα καρτεσιανό επίπεδο.
// - Η αρχή των αξόνων είναι η θέση του διαστημόπλοιου στην αρχή του παιχνιδιού
// - Στο άξονα x οι συντεταγμένες μεγαλώνουν προς τα δεξιά.
// - Στον άξονα y οι συντεταγμένες μεγαλώνουν προς τα πάνω.

static void add_asteroids(State state, int num) {
	for (int i = 0; i < num; i++) {
		// Τυχαία θέση σε απόσταση [ASTEROID_MIN_DIST, ASTEROID_MAX_DIST]
		// από το διστημόπλοιο.
		//
		Vector2 position = vec2_add(
			state->info.spaceship->position,
			vec2_from_polar(
				randf(ASTEROID_MIN_DIST, ASTEROID_MAX_DIST),	// απόσταση
				randf(0, 2*PI)									// κατεύθυνση
			)
		);

		// Τυχαία ταχύτητα στο διάστημα [ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED]
		// με τυχαία κατεύθυνση.
		//
		Vector2 speed = vec2_from_polar(
			randf(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED) * state->speed_factor,
			randf(0, 2*PI)
		);

		Object asteroid = create_object(
			ASTEROID,
			position,
			speed,
			(Vector2){0, 0},								// δεν χρησιμοποιείται για αστεροειδείς
			randf(ASTEROID_MIN_SIZE, ASTEROID_MAX_SIZE)		// τυχαίο μέγεθος
		);
		vector_insert_last(state->objects, asteroid);
	}
}

void vector_remove_at(Vector vector, int pos) {
	Pointer value_to_remove = vector_get_at(vector, pos);
	Pointer last_value = vector_get_at(vector, vector_size(vector)-1);
	Pointer temp;
	temp = last_value;
	last_value = value_to_remove;
	value_to_remove = temp;
	vector_remove_last(vector);
}
// Δημιουργεί και επιστρέφει την αρχική κατάσταση του παιχνιδιού
State state_create() {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	// Γενικές πληροφορίες
	state->info.paused = false;				// Το παιχνίδι ξεκινάει χωρίς να είναι paused.
	state->speed_factor = 1;				// Κανονική ταχύτητα
	state->next_bullet = 0;					// Σφαίρα επιτρέπεται αμέσως
	state->info.score = 0;				// Αρχικό σκορ 0

	// Δημιουργούμε το vector των αντικειμένων, και προσθέτουμε αντικείμενα
	state->objects = vector_create(0, NULL);

	// Δημιουργούμε το διαστημόπλοιο
	state->info.spaceship = create_object(
		SPACESHIP,
		(Vector2){0, 0},			// αρχική θέση στην αρχή των αξόνων
		(Vector2){0, 0},			// μηδενική αρχική ταχύτητα
		(Vector2){0, 1},			// κοιτάει προς τα πάνω
		SPACESHIP_SIZE				// μέγεθος
	);

	// Προσθήκη αρχικών αστεροειδών
	add_asteroids(state, ASTEROID_NUM);

	return state;
}

// Επιστρέφει τις βασικές πληροφορίες του παιχνιδιού στην κατάσταση state

StateInfo state_info(State state) {
	return &state->info;
}

//ελέγχει αν το σημείο στη θέση position είναι εντός του ορθογωνίου που ορίζουν 
//τα σημεία top_left και bottom_right
bool is_inside_rectangle(Vector2 position, Vector2 top_left, Vector2 bottom_right) {
	return position.x >= top_left.x && 
		   position.x <= bottom_right.x &&
		   position.y <= top_left.y &&
		   position.y >= bottom_right.y;
}

// Επιστρέφει μια λίστα με όλα τα αντικείμενα του παιχνιδιού στην κατάσταση state,
// των οποίων η θέση position βρίσκεται εντός του παραλληλογράμμου με πάνω αριστερή
// γωνία top_left και κάτω δεξιά bottom_right.
List state_objects(State state, Vector2 top_left, Vector2 bottom_right) {
	List object_list = list_create(NULL);
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object current_object = vector_get_at(state->objects, i);
		if (is_inside_rectangle(current_object->position, top_left, bottom_right)) 
			list_insert_next(object_list, list_last(object_list), current_object);
	}
	return object_list;
}

// Ενημερώνει την κατάσταση του διαστημοπλοίου
void spaceship_update(Object spaceship, KeyState keys, double speed_factor) {
	//περιστροφή διαστημοπλοίου
	if (keys->left || keys->right) {
		double rotation_angle = (keys->left ? +1 : -1) * SPACESHIP_ROTATION;
		spaceship->orientation = vec2_rotate(spaceship->orientation, rotation_angle);
	}

	// Μετατόπιση διαστημοπλοίου
	spaceship->position = vec2_add(spaceship->position, vec2_scale(spaceship->speed, speed_factor));
	
	if (keys->up) {  //επιτάχυνση διαστημοπλοίου
		Vector2 acceleration = vec2_scale(spaceship->orientation, SPACESHIP_ACCELERATION);
		spaceship->speed = vec2_add(spaceship->speed, acceleration);
	} else if (spaceship->speed.x > 0) {        //επιβράδυνση διαστημοπλοίου
		Vector2 slowdown = vec2_scale(spaceship->orientation, -SPACESHIP_SLOWDOWN);
		spaceship->speed = vec2_add(spaceship->speed, slowdown);
		//το διαστημόπλοιο ακινητοποιείται
		if (spaceship->speed.x < 0 || spaceship->speed.y < 0) {
			spaceship->speed = (Vector2){0,0};
		}
	}
}

// Ενημερώνει την κατάσταση του αντικειμένου (αστεροειδούς-σφαίρας) object
void object_update(Object object, int speed_factor) {
	// Μετατόπιση αντικειμένου
	object->position = vec2_add(object->position, vec2_scale(object->speed, speed_factor));
}

// Ενημερώνει την κατάσταση του παιχνιδιού - paused / not paused
void game_update(State state, bool togglePause) {
	if (togglePause) {
		state->info.paused = !state->info.paused;
	} 
}

// Ελέγχει αν ο αστεροειδής απέχει λιγότερο από ASTEROID_MAX_DIST από το διαστημόπλοιο
bool is_near_spaceship(Object object, Vector2 spaceship_position) {
	return vec2_distance(object->position, spaceship_position) < ASTEROID_MAX_DIST;
}

// Προσθέτει αστεροειδείς ώστε να υπάρχουν τουλάχιστον ASTEROID_NUM κοντά στο διαστημόπλοιο
void num_asteroids_update(State state) {
	int asteroids_near_spaceship = 0;
	
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object current_object = vector_get_at(state->objects, i);
		if (current_object->type != ASTEROID) continue;
		if (is_near_spaceship(current_object, state->info.spaceship->position)) {
			asteroids_near_spaceship++;
		}
	}

	add_asteroids(state, ASTEROID_NUM - asteroids_near_spaceship);
}

// Προσθέτει μία σφαίρα, μόνο αν έχουν περάσει τουλάχιστον BULLET_DELAY frames από την προηγούμενη
void add_bullet(State state) {
	if (state->next_bullet > 0) return;
	Object spaceship = state->info.spaceship;
	
	// Η σφαίρα έχει σχετική ταχύτητα BULLET_SPEED ως προς το διαστημόπλοιο
	Vector2 speed = vec2_add(spaceship->speed, vec2_scale(spaceship->orientation, BULLET_SPEED));
	// Η σφαίρα αρχικά βρίσκεται στη θέση του διαστημοπλοίου
	Vector2 position = spaceship->position;

	vector_insert_last(state->objects, create_object(BULLET, position, speed, (Vector2){0, 0}, BULLET_SIZE));
	state->next_bullet = BULLET_DELAY;
}

// Ελέγχει αν τα αντικέιμενα a και b (θεωρούνται σφαιρικά κέντρου object->position και ακτίνας object->size) συγκρούονται
bool collapses(Object a, Object b) {
	return CheckCollisionCircles(a->position, a->size, b->position, b->size);
}

// Βοηθητική συνάρτηση για τη handle_collisions:
// Δημιουγεί έναν νέο αστεροειδή με συγκεκριμένη ταχύτητα και μέγεθος σε τυχαία απόσταση από το διαστημόπλοιο
Object create_new_asteroid(int size, int speed_value, Vector2 spaceship_position) {
	// Η ταχύτητα έχει ορισμένο μέτρο και τυχαία κατεύθυνση
	Vector2 speed = vec2_from_polar(
		speed_value,
		randf(0, 2*PI)
	);

	Vector2 position = vec2_add(
		spaceship_position,
		vec2_from_polar(
			randf(ASTEROID_MIN_DIST, ASTEROID_MAX_DIST),
			randf(0, 2*PI)									
		)
	);

	Object new_asteroid = create_object(
		ASTEROID,
		position,
		speed,
		(Vector2){0,0},
		size
	);

	return new_asteroid;
}

void handle_bullet_asteroid_collision(State state, Object asteroid, int pos) {
	double initial_size = asteroid->size;
	double initial_speed = vec2_distance((Vector2){0,0}, asteroid->speed);
	state->info.score -= 10;

	if (initial_size >= 2*ASTEROID_MIN_SIZE) {
		for (int i = 0; i < 2; i++) 
			vector_insert_last(state->objects, create_new_asteroid(initial_size / 2, initial_speed * 1.5, state->info.spaceship->position));
		state->info.score += 2;
	}
	vector_remove_at(state->objects, pos);
}

void handle_asteroid_spaceship_collision(State state, int pos) {
	vector_remove_at(state->objects, pos);
	state->info.score /= 2;
}

// Εντοπίζει και διαχειρίζεται τις συγκρούσεις αστεροειδούς-διαστημοπλοίου και αστεροειδούς-σφαίρας
void handle_collisions(State state) {
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object object = vector_get_at(state->objects, i);
		if (object->type == ASTEROID && collapses(state->info.spaceship, object)) {
			// Έξαφανίζουμε τον αστεροειδή μηδενίζοντας το μέγεθός του
			object->size = 0;
			// Για κάθε σύγκρουση αστεροειδούς-διαστημοπλοίου το σκορ υποδιπλασιάζεται
			state->info.score /= 2;
		}
	}

	for (int i = 0; i < vector_size(state->objects); i++) {
		if (((Object)vector_get_at(state->objects, i))->type != ASTEROID) continue;
		Object asteroid = vector_get_at(state->objects, i);
		for (int j = 0; j < vector_size(state->objects); j++) {
			if (((Object)vector_get_at(state->objects, j))->type != BULLET) continue;	
			Object bullet = vector_get_at(state->objects, j);
			if (collapses(asteroid, bullet)) {
				handle_bullet_asteroid_collision(state, asteroid, i);
			}

			}
		if (collapses(asteroid, state->info.spaceship)) {
			handle_asteroid_spaceship_collision(state, i);
		}		
	}
}


// Ενημερώνει την κατάσταση state του παιχνιδιού μετά την πάροδο 1 frame.
// Το keys περιέχει τα πλήκτρα τα οποία ήταν πατημένα κατά το frame αυτό.
void state_update(State state, KeyState keys) {
	// Αλλάζει την κατάσταση του παιχνιδιού (paused - not paused) αν το p είναι πατημένο
	game_update(state, keys->p);

	if (state->info.paused) return;

	// Συγκρούσεις
	handle_collisions(state);
	
	// Ενημέρωση κατάστασης αντικειμένων
	for (int i = 0; i < vector_size(state->objects); i++) {
		Object current_object = vector_get_at(state->objects, i);
		object_update(current_object, state->speed_factor);
	}

	// Ενημέρωση κατάστασης διαστημοπλοίου
	spaceship_update(state_info(state)->spaceship, keys, state->speed_factor);
	
	// Προσθήκη νέων αστεροειδών
	num_asteroids_update(state);
	
	// Προσθήκη νέας σφαίρας
	if (keys->space) {
		add_bullet(state);
	}
	// Μείωση του αριθμού frames που απαιτούνται για να επιτραπεί η επόμενη σφαίρα
	state->next_bullet--;

	// Αύξηση της ταχύτητας παιχνιδιού αν το σκορ φτάσει σε κάποιο πολλαπλάσιο του 100
	if (state != 0 && state->info.score % 100 == 0) {
		state->speed_factor *= 1.1;
	}
	game_update(state, state->info.paused && keys->n);
}

// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη
void state_destroy(State state) {
	for (int i = 0; i < vector_size(state->objects); i++) {
		free(vector_get_at(state->objects, i));
	}
	vector_destroy(state->objects);
	free(state->info.spaceship);
	free(state);
}
