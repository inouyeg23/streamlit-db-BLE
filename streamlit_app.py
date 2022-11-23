import streamlit as st
from google.cloud import firestore
import json
from google.oauth2.service_account import Credentials
import pandas as pd
import math
import plotly.express as px
import numpy as np

st.set_page_config(page_title="CS 427 IoT Project", initial_sidebar_state="collapsed", layout="wide")

#db = firestore.Client.from_service_account_json("firestore-key.json")

key_dict = json.loads(st.secrets["textkey"])
creds = Credentials.from_service_account_info(key_dict)
db = firestore.Client(credentials=creds, project="streamlit-db")

crowd_data = []

crowd_ref = db.collection("crowd").document("crowd_doc").collection("crowd_col")
for doc in crowd_ref.stream():
	post = doc.to_dict()
	try:
		if len(post["dayoftheweek"]) != 0:
			dotw = post["dayoftheweek"]
			time = post["time"]
			num = post["num"]
			crowd_data.append([dotw, int(time), num])
	except:
		continue

	

def convert_num_to_time(i):
	if i > 999:
		i = str(i)
		return i[:2] + ":" + i[2:]
	elif i > 99:
		i = str(i)
		return i[:1] + ":" + i[1:]
	else:
		i = str(i)
		return  ":" + i
	
def times():
	timelist = [0] * 48
	n = 0
	for x in range(48):
		timelist[x] = convert_num_to_time(n)
		if n % 100 == 0:
			n+=30
		else:
			n+=70
	return timelist 

s_data = [["Monday", 1022, 43], ["Monday", 1023, 30], ["Tuesday", 1034, 45], ["Sunday", 1122, 42], 
		["Tuesday", 1123, 30], ["Tuesday", 1234, 45], ["Monday", 1122, 43], ["Tuesday", 1223, 30], 
		["Monday", 1334, 45], ["Tuesday", 1234, 45], ["Sunday", 1122, 43], ["Wednesday", 1223, 30],
		["Tuesday", 1234, 45], ["Monday", 1122, 43], ["Thursday", 1223, 30], ["Saturday", 1234, 45], 
		["Monday", 1122, 43], ["Friday", 1223, 30], ["Sunday", 1223, 30]]

data = crowd_data

button1, button2, button3 = st.columns(3)
with button1:
	sample_data = st.button("Use Sample Data",  key="sample data")
	if sample_data:
		data = s_data
with button2:
	db_data = st.button("Use Live Data", key="live data")
	if db_data:
		data=crowd_data
with button3:
	refresh = st.button("Refresh Data", key="refesh data")
	if refresh:
		st.experimental_rerun()

testdf = pd.DataFrame(data, columns=['day', 'time', 'num'])
#st.write(testdf)

dotw_df = [0] * 7
dotw = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]

chart_df = pd.DataFrame(columns=['dayoftheweek', 'time_range', 'avg_num'])
temp_df = chart_df

temp_df['time_range'] = times()
for x in range(7):
	gb = testdf.groupby(testdf['day'])
	try:
		dotw_df[x] = gb.get_group(dotw[x])
	except:
		continue
	
	
	specific_day_df = dotw_df[x]

	numdevices = [0] * 48
	starttime = 0
	endtime = 30

	for y in range(48):
		mask = (specific_day_df['time'] >= starttime) & (specific_day_df['time'] < endtime)
		starttime = endtime
		if endtime % 100 == 0:
			endtime+=30
		else:
			endtime+=70
	
		df = specific_day_df[mask]

		mean = df['num'].mean()
		if math.isnan(mean) == False:
			numdevices[y] = mean

	temp_df['avg_num'] = numdevices
	temp_df = temp_df.assign(dayoftheweek=dotw[x])
	chart_df = pd.concat([chart_df, temp_df])

chart_df.reset_index(drop=True, inplace=True)
chart_df.drop(index=chart_df.index[:48], axis=0, inplace=True)

fig2 = px.bar(chart_df, x="time_range", y="avg_num", color="dayoftheweek", 
				labels={
                     "time_range": "Time",
                     "avg_num": "Number of People",
                     "dayoftheweek": "Day of the Week"
                 },title="How Crowded is it?")


col1, col2 = st.columns([1,3], gap="small")
with col1:
	st.write(chart_df)
with col2:
	st.plotly_chart(fig2, use_container_width=True)