<?xml version="1.0" encoding="UTF-8"?>
<simconf version="2022112801">
  <simulation>
    <title>My simulation</title>
    <randomseed>generated</randomseed>
    <motedelay_us>1000000</motedelay_us>
    <radiomedium>
      org.contikios.cooja.radiomediums.UDGM
      <transmitting_range>100.0</transmitting_range>
      <interference_range>150.0</interference_range>
      <success_ratio_tx>1.0</success_ratio_tx>
      <success_ratio_rx>1.0</success_ratio_rx>
    </radiomedium>
    <events>
      <logoutput>40000</logoutput>
    </events>
    <motetype>
      org.contikios.cooja.mspmote.Z1MoteType
      <description>Z1 Mote Type #1</description>
      <source>[CONFIG_DIR]/car-client.c</source>
      <commands>make -j$(CPUS) car-client.z1 TARGET=z1</commands>
      <firmware>[CONFIG_DIR]/build/z1/car-client.z1</firmware>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspClock</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspButton</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.Msp802154Radio</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDefaultSerial</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspLED</moteinterface>
      <moteinterface>org.contikios.cooja.mspmote.interfaces.MspDebugOutput</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="301.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>1</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="311.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>2</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="321.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>3</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="331.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>4</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="341.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>5</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="351.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>6</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="361.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>7</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="371.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>8</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="381.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>9</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="391.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>10</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="401.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>11</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="411.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>12</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="421.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>13</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="431.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>14</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="441.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>15</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="451.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>16</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="461.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>17</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="471.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>18</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="481.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>19</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="491.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>20</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="501.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>21</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="511.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>22</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="521.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>23</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="531.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>24</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="541.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>25</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="551.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>26</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="561.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>27</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="571.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>28</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="581.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>29</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="591.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>30</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="601.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>31</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="611.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>32</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="621.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>33</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="631.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>34</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="641.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>35</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="651.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>36</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="661.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>37</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="671.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>38</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="681.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>39</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="691.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>40</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="701.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>41</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="711.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>42</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="721.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>43</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="731.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>44</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="741.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>45</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="751.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>46</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="761.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>47</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="771.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>48</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="781.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>49</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="791.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>50</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="801.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>51</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="811.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>52</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="821.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>53</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="831.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>54</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="841.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>55</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="851.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>56</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="861.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>57</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="871.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>58</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="881.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>59</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="891.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>60</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="901.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>61</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="911.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>62</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="921.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>63</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="931.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>64</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="941.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>65</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="951.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>66</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="961.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>67</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="971.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>68</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="981.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>69</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="991.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>70</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1001.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>71</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1011.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>72</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1021.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>73</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1031.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>74</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1041.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>75</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1051.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>76</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1061.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>77</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1071.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>78</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1081.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>79</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1091.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>80</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1101.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>81</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1111.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>82</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1121.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>83</id>
        </interface_config>
      </mote>
     <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1131.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>84</id>
        </interface_config>
      </mote>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="1141.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.mspmote.interfaces.MspMoteID
          <id>85</id>
        </interface_config>
      </mote>
    </motetype>
    <motetype>
      org.contikios.cooja.contikimote.ContikiMoteType
      <description>Cooja Mote Type #1</description>
      <source>[CONFIG_DIR]/rsu-server.c</source>
      <commands>make -j$(CPUS) rsu-server.cooja TARGET=cooja</commands>
      <moteinterface>org.contikios.cooja.interfaces.Position</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Battery</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiVib</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiMoteID</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRS232</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiBeeper</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.RimeAddress</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.IPAddress</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiRadio</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiButton</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiPIR</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiClock</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiLED</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiCFS</moteinterface>
      <moteinterface>org.contikios.cooja.contikimote.interfaces.ContikiEEPROM</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.Mote2MoteRelations</moteinterface>
      <moteinterface>org.contikios.cooja.interfaces.MoteAttributes</moteinterface>
      <mote>
        <interface_config>
          org.contikios.cooja.interfaces.Position
          <pos x="500.0" y="500.0" />
        </interface_config>
        <interface_config>
          org.contikios.cooja.contikimote.interfaces.ContikiMoteID
          <id>86</id>
        </interface_config>
      </mote>
    </motetype>
  </simulation>
  <plugin>
    org.contikios.cooja.plugins.Visualizer
    <plugin_config>
      <moterelations>true</moterelations>
      <skin>org.contikios.cooja.plugins.skins.IDVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.GridVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.TrafficVisualizerSkin</skin>
      <skin>org.contikios.cooja.plugins.skins.UDGMVisualizerSkin</skin>
      <viewport>0.09165719474906525 0.0 0.0 0.09165719474906525 286.7383913023021 177.5258396026288</viewport>
    </plugin_config>
    <bounds x="1" y="1" height="400" width="400" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.LogListener
    <plugin_config>
      <filter />
      <formatted_time />
      <coloring />
    </plugin_config>
    <bounds x="400" y="160" height="240" width="1308" z="6" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.TimeLine
    <plugin_config>
      <mote>0</mote>
      <mote>1</mote>
      <mote>2</mote>
      <mote>3</mote>
      <mote>4</mote>
      <mote>5</mote>
      <mote>6</mote>
      <mote>7</mote>
      <mote>8</mote>
      <mote>9</mote>
      <mote>10</mote>
      <mote>11</mote>
      <mote>12</mote>
      <mote>13</mote>
      <mote>14</mote>
      <mote>15</mote>
      <mote>16</mote>
      <mote>17</mote>
      <mote>18</mote>
      <mote>19</mote>
      <mote>20</mote>
      <mote>21</mote>
      <mote>22</mote>
      <mote>23</mote>
      <mote>24</mote>
      <mote>25</mote>
      <mote>26</mote>
      <mote>27</mote>
      <mote>28</mote>
      <mote>29</mote>
      <mote>30</mote>
      <mote>31</mote>
      <mote>32</mote>
      <mote>33</mote>
      <mote>34</mote>
      <mote>35</mote>
      <mote>36</mote>
      <mote>37</mote>
      <mote>38</mote>
      <mote>39</mote>
      <mote>40</mote>
      <mote>41</mote>
      <mote>42</mote>
      <mote>43</mote>
      <mote>44</mote>
      <mote>45</mote>
      <mote>46</mote>
      <mote>47</mote>
      <mote>48</mote>
      <mote>49</mote>
      <mote>50</mote>
      <mote>51</mote>
      <mote>52</mote>
      <mote>53</mote>
      <mote>54</mote>
      <mote>55</mote>
      <mote>56</mote>
      <mote>57</mote>
      <mote>58</mote>
      <mote>59</mote>
      <mote>60</mote>
      <mote>61</mote>
      <showRadioRXTX />
      <showRadioHW />
      <showLEDs />
      <zoomfactor>500.0</zoomfactor>
    </plugin_config>
    <bounds x="0" y="771" height="166" width="1708" z="5" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Notes
    <plugin_config>
      <notes>Enter notes here</notes>
      <decorations>true</decorations>
    </plugin_config>
    <bounds x="680" y="0" height="160" width="1028" z="4" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.Mobility
    <plugin_config>
      <positions>[CONFIG_DIR]/scenario_85_vehicles_positions.dat</positions>
    </plugin_config>
    <bounds x="27" y="485" height="200" width="500" z="3" />
  </plugin>
  <plugin>
    org.contikios.cooja.plugins.ScriptRunner
    <plugin_config>
      <scriptfile>[CONFIG_DIR]/coojalogger.js</scriptfile>
      <active>true</active>
    </plugin_config>
    <bounds x="897" y="40" height="700" width="600" z="2" />
  </plugin>
</simconf>
