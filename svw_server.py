#!/usr/bin/env python3
import os
import hashlib
import uuid
from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import pymysql

app = Flask(__name__, static_folder='.')
CORS(app)

DB_CONFIG = {
    'host': '127.0.0.1',
    'port': 3306,
    'user': 'mangos',
    'password': 'mangos',
    'charset': 'utf8mb4',
    'cursorclass': pymysql.cursors.DictCursor
}

RACES = {
    1: 'Human', 2: 'Orc', 3: 'Dwarf', 4: 'Night Elf',
    5: 'Undead', 6: 'Tauren', 7: 'Gnome', 8: 'Troll'
}

CLASSES = {
    1: {'name': 'Warrior', 'color': '#C79C6E'},
    2: {'name': 'Paladin', 'color': '#F58CBA'},
    3: {'name': 'Hunter', 'color': '#ABD473'},
    4: {'name': 'Rogue', 'color': '#FFF569'},
    5: {'name': 'Priest', 'color': '#FFFFFF'},
    7: {'name': 'Shaman', 'color': '#0070DE'},
    8: {'name': 'Mage', 'color': '#69CCF0'},
    9: {'name': 'Warlock', 'color': '#9482C9'},
    11: {'name': 'Druid', 'color': '#FF7D0A'}
}

ZONES = {
    1: 'Dun Morogh', 12: 'Elwynn Forest', 14: 'Durotar', 17: 'The Barrens',
    33: 'Stranglethorn Vale', 40: 'Westfall', 44: 'Redridge Mountains',
    45: 'Arathi Highlands', 47: 'The Hinterlands', 85: 'Tirisfal Glades',
    139: 'Eastern Plaguelands', 141: 'Teldrassil', 215: 'Mulgore',
    400: 'Thousand Needles', 405: 'Desolace', 440: 'Tanaris',
    490: 'Un\'Goro Crater', 1377: 'Silithus', 1519: 'Stormwind City',
    1537: 'Ironforge', 1637: 'Orgrimmar', 1638: 'Thunder Bluff',
    1497: 'Undercity', 1657: 'Darnassus'
}

def get_db_connection(db_name):
    config = DB_CONFIG.copy()
    config['db'] = db_name
    return pymysql.connect(**config)

def hash_password(username, password):
    auth_str = f"{username.upper()}:{password.upper()}"
    return hashlib.sha1(auth_str.encode('utf-8')).hexdigest().upper()

@app.route('/')
def index():
    return send_from_directory('.', 'svw.html')

@app.route('/api/server-info', methods=['GET'])
def server_info():
    try:
        conn = get_db_connection('realmd')
        with conn.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) as count FROM account")
            acc_count = cursor.fetchone()['count']
        conn.close()

        conn_char = get_db_connection('character0')
        with conn_char.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) as count FROM characters")
            char_count = cursor.fetchone()['count']
            cursor.execute("SELECT COUNT(*) as count FROM characters WHERE online = 1")
            online_count = cursor.fetchone()['count']
        conn_char.close()

        return jsonify({
            'success': True,
            'server_name': 'Solo-Vanilla-WoW',
            'realm_name': 'Vanilla AI Realm 1',
            'expansion': 'Vanilla 1.12.1',
            'total_accounts': acc_count,
            'total_characters': char_count,
            'online_characters': online_count,
            'status': 'ONLINE'
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/register', methods=['POST'])
def register():
    data = request.json or {}
    username = data.get('username', '').strip()
    password = data.get('password', '').strip()
    email = data.get('email', '').strip()

    if not username or not password:
        return jsonify({'success': False, 'error': 'Username and Password are required.'}), 400

    if len(username) < 3 or len(username) > 16:
        return jsonify({'success': False, 'error': 'Username must be between 3 and 16 characters.'}), 400

    if len(password) < 4 or len(password) > 32:
        return jsonify({'success': False, 'error': 'Password must be between 4 and 32 characters.'}), 400

    sha_pass = hash_password(username, password)

    try:
        conn = get_db_connection('realmd')
        with conn.cursor() as cursor:
            cursor.execute("SELECT id FROM account WHERE username = %s", (username.upper(),))
            if cursor.fetchone():
                conn.close()
                return jsonify({'success': False, 'error': f"Account '{username}' already exists!"}), 400

            cursor.execute(
                "INSERT INTO account (username, sha_pass_hash, email, joindate, expansion) VALUES (%s, %s, %s, NOW(), 0)",
                (username.upper(), sha_pass, email)
            )
            account_id = cursor.lastrowid
            cursor.execute("INSERT INTO realmcharacters (realmid, acctid, numchars) VALUES (1, %s, 0)", (account_id,))
            conn.commit()
        conn.close()

        return jsonify({
            'success': True,
            'message': f"Account '{username.upper()}' created successfully! You can now log in."
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/login', methods=['POST'])
def login():
    data = request.json or {}
    username = data.get('username', '').strip()
    password = data.get('password', '').strip()

    if not username or not password:
        return jsonify({'success': False, 'error': 'Username and Password required.'}), 400

    sha_pass = hash_password(username, password)

    try:
        conn = get_db_connection('realmd')
        with conn.cursor() as cursor:
            cursor.execute(
                "SELECT id, username, sha_pass_hash, gmlevel, email, joindate, last_ip, last_login, locked, expansion FROM account WHERE username = %s",
                (username.upper(),)
            )
            acc = cursor.fetchone()

        conn.close()

        if not acc or acc['sha_pass_hash'] != sha_pass:
            return jsonify({'success': False, 'error': 'Invalid username or password.'}), 401

        gm_titles = {0: 'Player', 1: 'Moderator', 2: 'Game Master', 3: 'Administrator'}

        account_info = {
            'id': acc['id'],
            'username': acc['username'],
            'email': acc['email'] or 'N/A',
            'gmlevel': acc['gmlevel'],
            'gm_title': gm_titles.get(acc['gmlevel'], 'Player'),
            'joindate': str(acc['joindate']) if acc['joindate'] else 'N/A',
            'last_login': str(acc['last_login']) if acc['last_login'] else 'N/A',
            'last_ip': acc['last_ip'] or '127.0.0.1',
            'locked': 'Locked' if acc['locked'] else 'Active',
            'expansion': 'Vanilla (1.12.1)'
        }

        return jsonify({'success': True, 'account': account_info})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/change-password', methods=['POST'])
def change_password():
    data = request.json or {}
    username = data.get('username', '').strip()
    old_password = data.get('old_password', '').strip()
    new_password = data.get('new_password', '').strip()

    if not username or not old_password or not new_password:
        return jsonify({'success': False, 'error': 'All fields are required.'}), 400

    if len(new_password) < 4 or len(new_password) > 32:
        return jsonify({'success': False, 'error': 'New password must be between 4 and 32 characters.'}), 400

    old_hash = hash_password(username, old_password)
    new_hash = hash_password(username, new_password)

    try:
        conn = get_db_connection('realmd')
        with conn.cursor() as cursor:
            cursor.execute("SELECT sha_pass_hash FROM account WHERE username = %s", (username.upper(),))
            acc = cursor.fetchone()

            if not acc or acc['sha_pass_hash'] != old_hash:
                conn.close()
                return jsonify({'success': False, 'error': 'Current password is incorrect.'}), 401

            cursor.execute(
                "UPDATE account SET sha_pass_hash = %s, v = '', s = '' WHERE username = %s",
                (new_hash, username.upper())
            )
            conn.commit()
        conn.close()

        return jsonify({'success': True, 'message': 'Password changed successfully!'})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/characters', methods=['GET'])
def get_characters():
    account_id = request.args.get('account_id')
    if not account_id:
        return jsonify({'success': False, 'error': 'account_id parameter is required.'}), 400

    try:
        conn = get_db_connection('character0')
        with conn.cursor() as cursor:
            cursor.execute(
                "SELECT guid, name, race, class, gender, level, money, zone, online, totaltime, stored_honorable_kills, honor_highest_rank FROM characters WHERE account = %s",
                (account_id,)
            )
            rows = cursor.fetchall()
        conn.close()

        chars = []
        for r in rows:
            copper = r['money'] % 100
            silver = (r['money'] // 100) % 100
            gold = r['money'] // 10000

            total_sec = r['totaltime']
            days = total_sec // 86400
            hours = (total_sec % 86400) // 3600
            mins = (total_sec % 3600) // 60
            played_str = f"{days}d {hours}h {mins}m" if days > 0 else f"{hours}h {mins}m"

            cls_info = CLASSES.get(r['class'], {'name': 'Unknown', 'color': '#CCCCCC'})

            chars.append({
                'guid': r['guid'],
                'name': r['name'],
                'level': r['level'],
                'race': RACES.get(r['race'], 'Unknown'),
                'class': cls_info['name'],
                'class_color': cls_info['color'],
                'gender': 'Male' if r['gender'] == 0 else 'Female',
                'gold': gold,
                'silver': silver,
                'copper': copper,
                'zone': ZONES.get(r['zone'], f"Zone #{r['zone']}"),
                'online': bool(r['online']),
                'played_time': played_str,
                'honorable_kills': r['stored_honorable_kills'],
                'pvp_rank': r['honor_highest_rank']
            })

        return jsonify({'success': True, 'characters': chars})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

if __name__ == '__main__':
    print("Starting Solo-Vanilla-WoW Account Manager Server on port 8080...")
    app.run(host='0.0.0.0', port=8080, debug=False)
