<template>
  <div class="md-layout md-gutter">
    <div class="md-layout-item md-small-size-100 md-size-50">
      <div class="md-elevation-3 pa-15 mt-10">
        <div class="md-display-3 text-center text-white" :style="{opacity: loading ? 0 : 1}">
          <md-icon class="md-size-2x text-red">thermostat</md-icon>
          {{data.t.toFixed(1)}}°C
        </div>
        <div class="md-caption text-center">Temperature</div>
      </div>
    </div>
    <div class="md-layout-item md-small-size-100 md-size-50">
      <div class="md-elevation-3 pa-15 mt-10">
        <div class="md-display-3 text-center text-white" :style="{opacity: loading ? 0 : 1}">
          <md-icon class="md-size-2x text-cyan">opacity</md-icon>
          {{data.h.toFixed(1)}}%
        </div>
        <div class="md-caption text-center">Humidity</div>
      </div>
    </div>
    <div class="md-layout-item md-small-size-100 md-size-50">
      <div class="md-elevation-3 pa-15 mt-10">
        <div class="md-display-3 text-center text-white" :style="{opacity: loading ? 0 : 1}">
          <md-icon class="md-size-2x" :class="{'text-amber': data.on}">lightbulb</md-icon>
          {{data.on ? 'ON' : 'OFF'}}
        </div>
        <div class="md-caption text-center">State</div>
      </div>
    </div>
    <div class="md-layout-item md-small-size-100 md-size-50">
      <div class="md-elevation-3 pa-15 mt-10">
        <div class="md-display-1 text-center text-white" :style="{opacity: loading ? 0 : 1}">
          <md-icon class="md-size-2x">history</md-icon>
          {{uptime}}
        </div>
        <div class="md-caption text-center">Uptime</div>
      </div>
    </div>
    <div class="md-layout-item md-small-size-100 md-size-50">
      <div class="md-layout md-elevation-3 pa-15 mt-10 md-alignment-top-left">
        <div class="md-layout-item md-size-10">
          <md-icon class="text-white">info</md-icon>
        </div>
        <div class="md-layout-item">
          <div>Model: {{chip_model[info.model]}}</div>
          <div>Revision: {{info.revision}}</div>
          <div>Cores: {{info.cores}}</div>
          <div>SDK-Version: {{info.version}}</div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import Service from '../service'

export default {
  name: 'Main',
  props: {
    
  },
  data(){
    return {
      loading: true,
      chip_model: {
        1: "ESP32",
        2: "ESP32-S2",
        4: "ESP32-S3",
        5: "ESP32-C3",
      },
      info: {},
      data: {},
    }
  },
  created(){
    this.getInfo()
    this.getData()
    setInterval(this.getData, 3000)
  },
  computed:{
    uptime(){
      let uptime = (typeof this.data.uptime == 'number') ? this.data.uptime : 0
      let time = this.partition(uptime/1000, [24*60*60, 60*60, 60, 1])
      return `${time[0]}d ${time[1]}h ${time[2]}m ${time[3]}s`
    }
  },
  methods: {
    getData(){
      Service.getData().then((res) => {
        this.data = res
        this.loading = false
      })
    },
    getInfo(){
      Service.getInfo().then((res) => {
        this.info = res 
      })
    },
    partition(value, partitions){
      let res = []
      for(let p of partitions){
        res.push(parseInt(value/p))
        value = value%p
      }
      return res
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>

</style>
